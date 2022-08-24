// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/statistics_view.h"

#include <pangomm/layout.h>

#include <sstream>
#include <string>
#include <utility>

#include "app/edit_date_dialog.h"
#include "app/filtered_activities_dialog.h"
#include "app/ui_helpers.h"
#include "app/utils.h"

namespace m_time_tracker {

namespace {

constexpr int kBarPadding = 10;
constexpr std::string_view kIntervalNone = "INTERVAL_NONE";
constexpr std::string_view kInterval24h = "INTERVAL_24H";
constexpr std::string_view kIntervalWeek = "INTERVAL_WEEK";
constexpr std::string_view kInterval30d = "INTERVAL_30D";
constexpr std::string_view kIntervalAll = "INTERVAL_ALL";

void SetDateToButton(
    const Activity::TimePoint time_point,
    Gtk::Button* date_button) noexcept {
  const std::tm local_time = TimePointToLocal(time_point);
  std::stringstream sstrm;
  sstrm << std::put_time(&local_time, "%Y %B %d");
  date_button->set_label(sstrm.str());
}

struct BarLayoutInfo {
  double bar_height;
  Glib::RefPtr<Pango::Layout> duration_layout;
  Glib::RefPtr<Pango::Layout> task_name_layout;
};

BarLayoutInfo MakeBarLayout(
    const Glib::RefPtr<Pango::Layout>& pango_layout,
    int control_width,
    const Activity::Duration& task_duration,
    const Task& task) noexcept {
  static constexpr int kMinTaskNameWidth = 25;

  const std::string duration_text =
      FormatRuntime(task_duration, FormatMode::kLongWithoutSeconds) + ": ";

  BarLayoutInfo result;
  result.duration_layout = pango_layout->copy();
  result.duration_layout->set_text(duration_text);

  const auto duration_rect = result.duration_layout->get_logical_extents();

  result.task_name_layout = pango_layout->copy();
  const int task_name_width =
      std::max(
          kMinTaskNameWidth * Pango::SCALE,
          (control_width - 2 * kBarPadding) * Pango::SCALE -
              duration_rect.get_width());
  result.task_name_layout->set_width(task_name_width);
  result.task_name_layout->set_text(task.name());

  const auto task_name_rect =
      result.task_name_layout->get_logical_extents();

  const double max_text_height = static_cast<double>(std::max(
      duration_rect.get_height(), task_name_rect.get_height()))
          / Pango::SCALE;

  result.bar_height = max_text_height + 2 * kBarPadding;

  return result;
}

}  // namespace

StatisticsView::StatisticsView(
    MainWindow* main_window,
    const Glib::RefPtr<Gtk::Builder>& resource_builder,
    AppState* app_state) noexcept
    : to_time_(GetLocalEndDayTimepoint(Activity::GetCurrentTimePoint())),
      from_time_(
          GetLocalStartDayTimepoint(to_time_ - std::chrono::hours(24))),
      main_window_(main_window),
      resource_builder_(resource_builder),
      app_state_(app_state) {
  InitializeWidgetPointers(resource_builder);
  SetDateToButton(from_time_, btn_from_);
  SetDateToButton(to_time_, btn_to_);

  btn_from_->signal_clicked().connect([this]() {
    EditDate(&from_time_);
    SetDateToButton(from_time_, btn_from_);
    Recalculate();
  });
  btn_to_->signal_clicked().connect([this]() {
    EditDate(&to_time_);
    SetDateToButton(to_time_, btn_to_);
    Recalculate();
  });
  drawing_->signal_draw().connect(
      sigc::mem_fun(*this, &StatisticsView::StatisticsDraw));
  drawing_->add_events(Gdk::BUTTON_PRESS_MASK);
  drawing_->signal_button_press_event().connect(
      sigc::mem_fun(*this, &StatisticsView::OnDrawingButtonPressed));
  existing_task_changed_connection_ = app_state_->ConnectExistingTaskChanged(
      sigc::mem_fun(*this, &StatisticsView::OnExistingTaskChanged));
  cmb_quick_select_->signal_changed().connect(
      sigc::mem_fun(*this, &StatisticsView::OnComboQuickSelectChanged));
}

StatisticsView::~StatisticsView() {
  existing_task_changed_connection_.disconnect();
}

void StatisticsView::InitializeWidgetPointers(
    const Glib::RefPtr<Gtk::Builder>& builder) noexcept {
  btn_from_ = GetWidgetChecked<Gtk::Button>(builder, "btn_stat_from");
  btn_to_ = GetWidgetChecked<Gtk::Button>(builder, "btn_stat_to");
  drawing_ = GetWidgetChecked<Gtk::DrawingArea>(builder, "drawing_stat");
  cmb_quick_select_ = GetWidgetChecked<Gtk::ComboBoxText>(
      builder, "cmb_quick_select");
}

void StatisticsView::EditDate(Activity::TimePoint* timepoint) noexcept {
  std::tm src_datetime = TimePointToLocal(*timepoint);
  Glib::RefPtr<EditDateDialog> edit_date_dialog =
      GetWindowDerived<EditDateDialog>(
          resource_builder_, "edit_date_dialog");
  edit_date_dialog->set_date(src_datetime);
  const int result = edit_date_dialog->run();
  edit_date_dialog->hide();
  if (result == Gtk::RESPONSE_OK) {
    std::tm new_date = edit_date_dialog->get_date();
    new_date.tm_hour = src_datetime.tm_hour;
    new_date.tm_min = src_datetime.tm_min;
    new_date.tm_sec = src_datetime.tm_sec;
    *timepoint = TimePointFromLocal(new_date);
  }
}

bool StatisticsView::StatisticsDraw(
    const Cairo::RefPtr<Cairo::Context>& ctx) noexcept {
  if (displayed_stats_.empty()) {
    return true;
  }
  Glib::RefPtr<Pango::Layout> pango_layout = Pango::Layout::create(ctx);
  SetFontForBarDrawing(pango_layout);

  Activity::Duration total_duration{};
  for (const DisplayedStatInfo& entry : displayed_stats_) {
    total_duration += entry.stat.duration;
  }
  const int control_width = drawing_->get_allocated_width();
  double current_y = 0.5;
  ctx->set_line_width(1);
  for (DisplayedStatInfo& entry : displayed_stats_) {
    const auto it_task = tasks_cache_.find(entry.stat.task_id);
    VERIFY(it_task != tasks_cache_.end());
    const Cairo::RectangleInt stat_entry_rect = DrawStatEntryRect(
        ctx,
        control_width,
        current_y,
        pango_layout,
        total_duration,
        entry.stat.duration,
        it_task->second);
    entry.last_drawn_rect = stat_entry_rect;
    current_y += stat_entry_rect.height;
  }
  return true;  // Don't invoke any other "draw" event handlers.
}

bool StatisticsView::OnDrawingButtonPressed(GdkEventButton* evt) noexcept {
  std::optional<Task> chosen_task;
  for (const DisplayedStatInfo& entry : displayed_stats_) {
    if (!entry.last_drawn_rect) {
      continue;
    }
    // Intentionally don't check X to ease clicking on too narrow bars.
    if (entry.last_drawn_rect->y <= evt->y &&
        (entry.last_drawn_rect->y + entry.last_drawn_rect->height) > evt->y) {
      const auto it_task = tasks_cache_.find(entry.stat.task_id);
      VERIFY(it_task != tasks_cache_.end());
      chosen_task = it_task->second;
      break;
    }
  }
  if (chosen_task) {
    if (!current_parent_task_id_ && HasChildren(*chosen_task)) {
      current_parent_task_id_ = chosen_task->id();
      Recalculate();
    } else {
      auto activities_list = Activity::LoadFiltered(
          &app_state_->db_for_read_only(),
          chosen_task->id(),
          from_time_,
          to_time_);
      // TODO(vchigrin): Better error handling.
      VERIFY(activities_list);
      Glib::RefPtr<FilteredActivitiesDialog> dlg =
          GetWindowDerived<FilteredActivitiesDialog>(
              resource_builder_, "filtered_activities_dialog", app_state_);
      dlg->SetActivitiesList(activities_list.value());
      dlg->run();
      dlg->hide();
      // User may have edited some of the activities.
      Recalculate();
    }
  } else if (current_parent_task_id_) {
    current_parent_task_id_ = std::nullopt;
    Recalculate();
  }
  return false;  // Allow event propagation.
}

void StatisticsView::SetFontForBarDrawing(
    const Glib::RefPtr<Pango::Layout>& pango_layout) noexcept {
  const Glib::RefPtr<Gtk::StyleContext> style_context =
      drawing_->get_style_context();
  const Pango::FontDescription font_description =
      style_context->get_font(style_context->get_state());
  pango_layout->set_font_description(font_description);
  pango_layout->set_ellipsize(Pango::ELLIPSIZE_END);
}

int StatisticsView::CalculageContentHeight() noexcept {
  const int control_width = drawing_->get_allocated_width();
  Glib::RefPtr<Pango::Layout> pango_layout = Pango::Layout::create(
      drawing_->get_pango_context());
  SetFontForBarDrawing(pango_layout);
  double result = 0;
  for (const DisplayedStatInfo& entry : displayed_stats_) {
    const auto it_task = tasks_cache_.find(entry.stat.task_id);
    VERIFY(it_task != tasks_cache_.end());
    const BarLayoutInfo layout_info = MakeBarLayout(
        pango_layout,
        control_width,
        entry.stat.duration,
        it_task->second);
    result += layout_info.bar_height;
  }
  return static_cast<int>(result);
}

Cairo::RectangleInt StatisticsView::DrawStatEntryRect(
    const Cairo::RefPtr<Cairo::Context>& ctx,
    int control_width,
    double current_y,
    const Glib::RefPtr<Pango::Layout>& pango_layout,
    const Activity::Duration& total_duration,
    const Activity::Duration& task_duration,
    const Task& task) const noexcept {
  const BarLayoutInfo layout_info = MakeBarLayout(
      pango_layout,
      control_width,
      task_duration,
      task);

  VERIFY(total_duration >= task_duration);
  const auto bar_width = (task_duration * control_width) / total_duration;

  const auto duration_rect =
      layout_info.duration_layout->get_logical_extents();

  ctx->rectangle(
      0.5, current_y, static_cast<double>(bar_width), layout_info.bar_height);
  ctx->save();
  // Light blue
  ctx->set_source_rgb(0, 128, 255);
  ctx->fill_preserve();
  ctx->restore();
  ctx->stroke();

  // Dark blue.
  ctx->set_source_rgb(0, 0, 255);
  double text_x =
      static_cast<double>(duration_rect.get_x()) / Pango::SCALE +
          kBarPadding;
  ctx->move_to(text_x, current_y + kBarPadding);
  layout_info.duration_layout->show_in_cairo_context(ctx);

  text_x += static_cast<double>(duration_rect.get_width()) / Pango::SCALE;
  // Black
  ctx->set_source_rgb(0, 0, 0);
  ctx->move_to(text_x, current_y + kBarPadding);
  layout_info.task_name_layout->show_in_cairo_context(ctx);
  Cairo::RectangleInt result;
  result.x = 0;
  result.y = static_cast<int>(current_y);
  result.width = control_width;
  result.height = static_cast<int>(layout_info.bar_height);
  return result;
}

void StatisticsView::Recalculate() noexcept {
  const outcome::std_result<std::vector<Activity::StatEntry>> maybe_stats =
      current_parent_task_id_ ?
          Activity::LoadStatsForInterval(
              &app_state_->db_for_read_only(),
              from_time_, to_time_,
              *current_parent_task_id_) :
          Activity::LoadStatsForTopLevelTasksInInterval(
              &app_state_->db_for_read_only(),
              from_time_, to_time_);
  // TODO(vchigrin): Better error handling.
  VERIFY(maybe_stats);
  displayed_stats_.clear();
  displayed_stats_.reserve(maybe_stats.value().size());
  for (const Activity::StatEntry& stat : maybe_stats.value()) {
    displayed_stats_.push_back({stat, std::nullopt});
  }
  std::sort(
      displayed_stats_.begin(),
      displayed_stats_.end(),
      [](const DisplayedStatInfo& first, const DisplayedStatInfo& second) {
      // Sort by duration, descending.
      return first.stat.duration > second.stat.duration;
  });
  for (const DisplayedStatInfo& entry : displayed_stats_) {
    if (tasks_cache_.count(entry.stat.task_id) == 0) {
      auto maybe_task = Task::LoadById(
          &app_state_->db_for_read_only(),
          entry.stat.task_id);
      // TODO(vchigrin): Better error handling.
      VERIFY(maybe_task);
      tasks_cache_.insert({
        entry.stat.task_id, std::move(maybe_task.value())});
    }
  }
  drawing_->set_size_request(-1, CalculageContentHeight());
  drawing_->queue_draw();
}

void StatisticsView::OnExistingTaskChanged(const Task& task) noexcept {
  VERIFY(task.id());
  auto it = tasks_cache_.find(*task.id());
  if (it != tasks_cache_.end()) {
    it->second = task;
  }
}

bool StatisticsView::HasChildren(const Task& task) const noexcept {
  VERIFY(task.id());
  const auto maybe_child_count = Task::ChildTasksCount(
      &app_state_->db_for_read_only(),
      *task.id());
  // TODO(vchigrin): Better error handling.
  VERIFY(maybe_child_count);
  return maybe_child_count.value() != 0;
}

void StatisticsView::OnComboQuickSelectChanged() noexcept {
  const std::string str_quick_select_id = cmb_quick_select_->get_active_id();
  VERIFY(!str_quick_select_id.empty());
  if (str_quick_select_id == kIntervalNone) {
    return;
  }
  to_time_ = Activity::GetCurrentTimePoint();
  // TODO(vchigrin): Use chrono::days in C++20.
  static constexpr auto kDay = std::chrono::hours(24);
  if (str_quick_select_id == kInterval24h) {
    from_time_ = to_time_ - std::chrono::hours(24);
  } else if (str_quick_select_id == kIntervalWeek) {
    from_time_ = to_time_ - kDay * 7;
  } else if (str_quick_select_id == kInterval30d) {
    from_time_ = to_time_ - kDay * 30;
  } else if (str_quick_select_id == kIntervalAll) {
    auto earliest_start_or_error = Activity::LoadEarliestActivityStart(
        &app_state_->db_for_read_only());
    // TODO(vchigrin): Better error handling.
    VERIFY(earliest_start_or_error);
    const std::optional<Activity::TimePoint> maybe_from =
        earliest_start_or_error.value();
    if (maybe_from) {
      from_time_ = *maybe_from;
    } else {
      // No activities yet.
      from_time_ = to_time_;
    }
  } else {
    VERIFY(false);  // Unexpected ID in combo box.
  }
  SetDateToButton(from_time_, btn_from_);
  SetDateToButton(to_time_, btn_to_);
  Recalculate();
  cmb_quick_select_->set_active_id(std::string(kIntervalNone));
}

}  // namespace m_time_tracker
