// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/edit_activity_dialog.h"

#include <charconv>

#include <boost/algorithm/string/trim.hpp>

#include "app/app_state.h"
#include "app/utils.h"

namespace m_time_tracker {

namespace {

void SetDateToControls(
    const std::tm& local_time,
    Gtk::Button* date_button) noexcept {
  std::stringstream sstrm;
  sstrm << std::put_time(&local_time, "%Y %B %d");
  date_button->set_label(sstrm.str());
}

void SetTimePointToConstrols(
    const std::tm& local_time,
    Gtk::SpinButton* spn_hours,
    Gtk::SpinButton* spn_minutes) noexcept {
  spn_hours->set_value(local_time.tm_hour);
  spn_minutes->set_value(local_time.tm_min);
}

void SetTimeFromConstrols(
    std::tm* local_time,
    Gtk::SpinButton* spn_hours,
    Gtk::SpinButton* spn_minutes) noexcept {
  local_time->tm_hour = spn_hours->get_value_as_int();
  local_time->tm_min = spn_minutes->get_value_as_int();
}

}  // namespace

EditActivityDialog::EditActivityDialog(
    GtkDialog* dlg,
    const Glib::RefPtr<Gtk::Builder>& resource_builder,
    AppState* app_state)
    : Gtk::Dialog(dlg),
      resource_builder_(resource_builder),
      app_state_(app_state) {
  InitializeWidgetPointers(resource_builder);
  btn_start_date_->signal_clicked().connect([this]() {
    EditDate(&start_time_);
    SetDateToControls(start_time_, btn_start_date_);
  });
  btn_end_date_->signal_clicked().connect([this]() {
    EditDate(&end_time_);
    SetDateToControls(end_time_, btn_end_date_);
  });
}

void EditActivityDialog::FillTasksCombo() noexcept {
  cmb_tasks_->remove_all();
  auto maybe_tasks = Task::LoadAll(&app_state_->db_for_read_only());
  // TODO(vchigrin): Better error handling.
  VERIFY(maybe_tasks);
  for (const Task& t : maybe_tasks.value()) {
    VERIFY(t.id());
    const std::string id = std::to_string(*t.id());
    cmb_tasks_->append(id, t.name());
  }
}

void EditActivityDialog::on_response(int response_id) {
  if (response_id == Gtk::RESPONSE_OK && activity_) {
    SetTimeFromConstrols(&start_time_, spn_start_hours_, spn_start_minutes_);
    SetTimeFromConstrols(&end_time_, spn_end_hours_, spn_end_minutes_);
    const Activity::TimePoint start_tp = TimePointFromLocal(start_time_);
    const Activity::TimePoint end_tp = TimePointFromLocal(end_time_);
    const std::string str_task_id = cmb_tasks_->get_active_id();
    VERIFY(!str_task_id.empty());
    Task::Id task_id = 0;
    auto [ptr, ec] =  std::from_chars(
        str_task_id.data(),
        str_task_id.data() + str_task_id.size(),
        task_id);
    VERIFY(ec == std::errc());
    VERIFY(ptr = str_task_id.data() + str_task_id.size());
    activity_->SetTaskId(task_id);
    activity_->SetInterval(start_tp, end_tp);
  }
}

void EditActivityDialog::on_show() {
  Gtk::Dialog::on_show();
  FillTasksCombo();
  if (activity_) {
    start_time_ = TimePointToLocal(activity_->start_time());
    // Don't expect not complete activities here.
    VERIFY(activity_->end_time());
    end_time_ = TimePointToLocal(*activity_->end_time());

    SetDateToControls(start_time_, btn_start_date_);
    SetDateToControls(end_time_, btn_end_date_);

    SetTimePointToConstrols(start_time_, spn_start_hours_, spn_start_minutes_);
    SetTimePointToConstrols(end_time_, spn_end_hours_, spn_end_minutes_);
    const bool task_found = cmb_tasks_->set_active_id(
        std::to_string(activity_->task_id()));
    VERIFY(task_found);
  }
}

void EditActivityDialog::InitializeWidgetPointers(
    const Glib::RefPtr<Gtk::Builder>& builder) noexcept {
  cmb_tasks_ = GetWidgetChecked<Gtk::ComboBoxText>(builder, "cmb_tasks");
  btn_start_date_ = GetWidgetChecked<Gtk::Button>(builder, "btn_start_date");
  spn_start_hours_ = GetWidgetChecked<Gtk::SpinButton>(
      builder, "spn_start_hours");
  spn_start_minutes_ = GetWidgetChecked<Gtk::SpinButton>(
      builder, "spn_start_minutes");
  btn_end_date_ = GetWidgetChecked<Gtk::Button>(builder, "btn_end_date");
  spn_end_hours_ = GetWidgetChecked<Gtk::SpinButton>(
      builder, "spn_end_hours");
  spn_end_minutes_ = GetWidgetChecked<Gtk::SpinButton>(
      builder, "spn_end_minutes");
}

void EditActivityDialog::EditDate(std::tm* datetime) noexcept {
  if (!edit_date_dialog_) {
    // Create dialog just once, then re-use it. If we destoy it GtkBuilder
    // will still attempt to return reference to the old object.
    edit_date_dialog_ =
      GetWindowDerived<EditDateDialog>(
          resource_builder_, "edit_date_dialog");
  }
  edit_date_dialog_->set_date(*datetime);
  const int result = edit_date_dialog_->run();
  edit_date_dialog_->hide();
  if (result == Gtk::RESPONSE_OK) {
    *datetime = edit_date_dialog_->get_date();
  }
}

}  // namespace m_time_tracker
