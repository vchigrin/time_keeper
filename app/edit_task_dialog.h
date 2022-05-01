// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "app/ui_helpers.h"
#include "app/task.h"

namespace m_time_tracker {

class AppState;

class EditTaskDialog : public Gtk::Dialog {
 public:
  EditTaskDialog(
      GtkDialog* dlg,
      const Glib::RefPtr<Gtk::Builder>& builder,
      AppState* app_state);

  void set_task(Task* task) noexcept {
    task_ = task;
  }

 protected:
  void on_response(int response_id) override;
  void on_show() override;

 private:
  void OnTaskNameChange() noexcept;
  std::string GetTrimmedEditText() noexcept;
  std::vector<Task> LoadChildTasks() noexcept;
  void InitializeParentTaskCombo(
      const std::vector<Task>& child_tasks) noexcept;

  Gtk::Entry* edt_task_name_ = nullptr;
  Gtk::Button* btn_ok_ = nullptr;
  Gtk::CheckButton* chk_archived_ = nullptr;
  Gtk::ComboBoxText* cmb_parent_task_ = nullptr;
  AppState* app_state_ = nullptr;
  Task* task_ = nullptr;
};

}  // namespace m_time_tracker
