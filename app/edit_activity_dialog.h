// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <utility>

#include "app/activity.h"
#include "app/edit_date_dialog.h"
#include "app/ui_helpers.h"

namespace m_time_tracker {

class AppState;
class MainWindow;

class EditActivityDialog : public Gtk::Dialog {
 public:
  EditActivityDialog(
      GtkDialog* dlg,
      const Glib::RefPtr<Gtk::Builder>& builder,
      AppState* app_state,
      MainWindow* main_window) noexcept;

  void set_activity(Activity* activity) noexcept {
    activity_ = activity;
  }

 protected:
  void on_response(int response_id) override;
  void on_show() override;

 private:
  void InitializeWidgetPointers(
      const Glib::RefPtr<Gtk::Builder>& builder) noexcept;
  void EditDate(std::tm* datetime) noexcept;
  void FillTasksCombo() noexcept;

  Activity* activity_ = nullptr;

  Gtk::ComboBoxText* cmb_tasks_ = nullptr;

  Gtk::Button* btn_start_date_ = nullptr;
  Gtk::SpinButton* spn_start_hours_ = nullptr;
  Gtk::SpinButton* spn_start_minutes_ = nullptr;

  Gtk::Button* btn_end_date_ = nullptr;
  Gtk::SpinButton* spn_end_hours_ = nullptr;
  Gtk::SpinButton* spn_end_minutes_ = nullptr;
  std::tm start_time_;
  std::tm end_time_;
  Glib::RefPtr<Gtk::Builder> resource_builder_;
  AppState* const app_state_;
  MainWindow* const main_window_;
};

}  // namespace m_time_tracker
