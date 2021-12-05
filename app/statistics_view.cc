// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/statistics_view.h"

#include <sstream>

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
  (void)ctx;
  return true;  // Don't invoke any other "draw" event handlers.
}

void StatisticsView::Recalculate() noexcept {
  // TODO(vchigrin): Load stats from DB.
}

}  // namespace m_time_tracker
