// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/statistics_view.h"

#include <pangomm/layout.h>

#include <sstream>
#include <string>
#include <utility>

#include "app/edit_date_dialog.h"
#include "app/ui_helpers.h"
#include "app/utils.h"

namespace m_time_tracker {

namespace {

void SetDateToButton(
    const Activity::TimePoint time_point,
    Gtk::Button* date_button) noexcept {
  const std::tm local_time = TimePointToLocal(time_point);
  std::stringstream sstrm;
  sstrm << std::put_time(&local_time, "%Y %B %d");
  date_button->set_label(sstrm.str());
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
  existing_task_changed_connection_ = app_state_->ConnectExistingTaskChanged(
      sigc::mem_fun(*this, &StatisticsView::OnExistingTaskChanged));
}

StatisticsView::~StatisticsView() {
  existing_task_changed_connection_.disconnect();
}

void StatisticsView::InitializeWidgetPointers(
    const Glib::RefPtr<Gtk::Builder>& builder) noexcept {
  btn_from_ = GetWidgetChecked<Gtk::Button>(builder, "btn_stat_from");
  btn_to_ = GetWidgetChecked<Gtk::Button>(builder, "btn_stat_to");
  drawing_ = GetWidgetChecked<Gtk::DrawingArea>(builder, "drawing_stat");
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

  const Glib::RefPtr<Gtk::StyleContext> style_context =
      drawing_->get_style_context();
  const Pango::FontDescription font_description =
      style_context->get_font(style_context->get_state());
  Glib::RefPtr<Pango::Layout> pango_layout = Pango::Layout::create(ctx);
  pango_layout->set_font_description(font_description);
  pango_layout->set_ellipsize(Pango::ELLIPSIZE_END);

  Activity::Duration total_duration{};
  for (const Activity::StatEntry& entry : displayed_stats_) {
    total_duration += entry.duration;
  }
  const int control_width = drawing_->get_allocated_width();
  double current_y = 0.5;
  ctx->set_line_width(1);
  for (const Activity::StatEntry& entry : displayed_stats_) {
    const auto it_task = tasks_cache_.find(entry.task_id);
    VERIFY(it_task != tasks_cache_.end());
    const Cairo::RectangleInt stat_entry_rect = DrawStatEntryRect(
        ctx,
        control_width,
        current_y,
        pango_layout,
        total_duration,
        entry.duration,
        it_task->second);
    current_y += stat_entry_rect.height;
  }
  return true;  // Don't invoke any other "draw" event handlers.
}

Cairo::RectangleInt StatisticsView::DrawStatEntryRect(
    const Cairo::RefPtr<Cairo::Context>& ctx,
    int control_width,
    double current_y,
    const Glib::RefPtr<Pango::Layout>& pango_layout,
    const Activity::Duration& total_duration,
    const Activity::Duration& task_duration,
    const Task& task) const noexcept {
  static constexpr int kBarPadding = 10;
  static constexpr int kMinTaskNameWidth = 25;

  VERIFY(total_duration >= task_duration);
  const auto bar_width = (task_duration * control_width) / total_duration;

  const std::string duration_text =
      FormatRuntime(task_duration, FormatMode::kLongWithoutSeconds) + ": ";

  const Glib::RefPtr<Pango::Layout> duration_layout = pango_layout->copy();
  duration_layout->set_text(duration_text);
  const auto duration_rect = duration_layout->get_logical_extents();

  const Glib::RefPtr<Pango::Layout> task_name_layout = pango_layout->copy();
  const int task_name_width =
      std::max(
          kMinTaskNameWidth * Pango::SCALE,
          (control_width - 2 * kBarPadding) * Pango::SCALE -
              duration_rect.get_width());
  task_name_layout->set_width(task_name_width);
  task_name_layout->set_text(task.name());
  const auto task_name_rect = task_name_layout->get_logical_extents();

  const double max_text_height = static_cast<double>(std::max(
      duration_rect.get_height(), task_name_rect.get_height()))
          / Pango::SCALE;

  const double bar_height = max_text_height + 2 * kBarPadding;

  ctx->rectangle(0.5, current_y, static_cast<double>(bar_width), bar_height);
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
  duration_layout->show_in_cairo_context(ctx);

  text_x += static_cast<double>(duration_rect.get_width()) / Pango::SCALE;
  // Black
  ctx->set_source_rgb(0, 0, 0);
  ctx->move_to(text_x, current_y + kBarPadding);
  task_name_layout->show_in_cairo_context(ctx);
  Cairo::RectangleInt result;
  result.x = 0;
  result.y = static_cast<int>(current_y);
  result.width = control_width;
  result.height = static_cast<int>(bar_height);
  return result;
}

void StatisticsView::Recalculate() noexcept {
  auto maybe_stats = Activity::LoadStatsForInterval(
      &app_state_->db_for_read_only(),
      from_time_, to_time_);
  // TODO(vchigrin): Better error handling.
  VERIFY(maybe_stats);
  displayed_stats_ = std::move(maybe_stats.value());
  std::sort(
      displayed_stats_.begin(),
      displayed_stats_.end(),
      [](const Activity::StatEntry& first, const Activity::StatEntry& second) {
      // Sort by duration, descending.
      return first.duration > second.duration;
  });
  drawing_->queue_draw();
  for (const Activity::StatEntry& entry : displayed_stats_) {
    if (tasks_cache_.count(entry.task_id) == 0) {
      auto maybe_task = Task::LoadById(
          &app_state_->db_for_read_only(),
          entry.task_id);
      // TODO(vchigrin): Better error handling.
      VERIFY(maybe_task);
      tasks_cache_.insert({entry.task_id, std::move(maybe_task.value())});
    }
  }
}

void StatisticsView::OnExistingTaskChanged(const Task& task) noexcept {
  VERIFY(task.id());
  auto it = tasks_cache_.find(*task.id());
  if (it != tasks_cache_.end()) {
    it->second = task;
  }
}
}  // namespace m_time_tracker
