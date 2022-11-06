// Copyright 2022 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/view_with_date_range.h"

#include <sstream>
#include <string>
#include <utility>

#include "app/edit_date_dialog.h"
#include "app/ui_helpers.h"
#include "app/utils.h"
#include "app/main_window.h"

namespace m_time_tracker {

namespace {

constexpr std::string_view kIntervalNone = "INTERVAL_NONE";
constexpr std::string_view kInterval24h = "INTERVAL_24H";
constexpr std::string_view kIntervalToday = "INTERVAL_TODAY";
constexpr std::string_view kIntervalWeek = "INTERVAL_WEEK";
constexpr std::string_view kInterval30d = "INTERVAL_30D";
constexpr std::string_view kIntervalAll = "INTERVAL_ALL";

void SetDateToButton(
    const Activity::TimePoint time_point,
    Gtk::Button* date_button) noexcept {
  const std::tm local_time = TimePointToLocal(time_point);
  std::stringstream sstrm;
  // Use system locale for date/time formatting.
  sstrm.imbue(std::locale(""));
  sstrm << std::put_time(&local_time, "%Y %b %d");
  date_button->set_label(sstrm.str());
}

}  // namespace

ViewWithDateRange::ViewWithDateRange(
    MainWindow* main_window,
    const Glib::RefPtr<Gtk::Builder>& resource_builder,
    AppState* app_state,
    const Glib::ustring& btn_stat_from_name,
    const Glib::ustring& btn_stat_to_name,
    const Glib::ustring& cmb_quick_select_name) noexcept
    : to_time_(GetLocalEndDayTimepoint(Activity::GetCurrentTimePoint())),
      from_time_(
          GetLocalStartDayTimepoint(to_time_ - std::chrono::hours(24))),
      main_window_(main_window),
      resource_builder_(resource_builder),
      app_state_(app_state) {
  InitializeWidgetPointers(
      resource_builder,
      btn_stat_from_name,
      btn_stat_to_name,
      cmb_quick_select_name);
  SetDateToButton(from_time_, btn_from_);
  SetDateToButton(to_time_, btn_to_);

  btn_from_->signal_clicked().connect([this]() {
    EditDate(&from_time_);
    SetDateToButton(from_time_, btn_from_);
    OnDateRangeChanged();
  });
  btn_to_->signal_clicked().connect([this]() {
    EditDate(&to_time_);
    SetDateToButton(to_time_, btn_to_);
    OnDateRangeChanged();
  });
  cmb_quick_select_->signal_changed().connect(
      sigc::mem_fun(*this, &ViewWithDateRange::OnComboQuickSelectChanged));
}

void ViewWithDateRange::InitializeWidgetPointers(
    const Glib::RefPtr<Gtk::Builder>& builder,
    const Glib::ustring& btn_stat_from_name,
    const Glib::ustring& btn_stat_to_name,
    const Glib::ustring& cmb_quick_select_name) noexcept {
  btn_from_ = GetWidgetChecked<Gtk::Button>(builder, btn_stat_from_name);
  btn_to_ = GetWidgetChecked<Gtk::Button>(builder, btn_stat_to_name);
  cmb_quick_select_ = GetWidgetChecked<Gtk::ComboBoxText>(
      builder, cmb_quick_select_name);
}

void ViewWithDateRange::EditDate(Activity::TimePoint* timepoint) noexcept {
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

void ViewWithDateRange::OnComboQuickSelectChanged() noexcept {
  const std::string str_quick_select_id = cmb_quick_select_->get_active_id();
  VERIFY(!str_quick_select_id.empty());
  if (str_quick_select_id == kIntervalNone) {
    return;
  }
  to_time_ = Activity::GetCurrentTimePoint();
  const auto this_day_start = GetLocalStartDayTimepoint(to_time_);
  // TODO(vchigrin): Use chrono::days in C++20.
  static constexpr auto kDay = std::chrono::hours(24);
  if (str_quick_select_id == kInterval24h) {
    from_time_ = to_time_ - std::chrono::hours(24);
  } else if (str_quick_select_id == kIntervalToday) {
    from_time_ = this_day_start;
  } else if (str_quick_select_id == kIntervalWeek) {
    from_time_ = this_day_start - kDay * 6;
  } else if (str_quick_select_id == kInterval30d) {
    from_time_ = this_day_start - kDay * 29;
  } else if (str_quick_select_id == kIntervalAll) {
    auto earliest_start_or_error = Activity::LoadEarliestActivityStart(
        &app_state_->db_for_read_only());
    if (!earliest_start_or_error) {
      main_window_->OnFatalError(earliest_start_or_error.assume_error());
    }
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
  OnDateRangeChanged();
  cmb_quick_select_->set_active_id(std::string(kIntervalNone));
}

}  // namespace m_time_tracker
