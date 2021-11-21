// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <utility>

#include "app/ui_helpers.h"
#include "app/task.h"

namespace m_time_tracker {

class EditTaskDialog : public Gtk::Dialog {
 public:
  EditTaskDialog(GtkDialog* dlg, const Glib::RefPtr<Gtk::Builder>& builder);

  void set_task(Task* task) noexcept {
    task_ = task;
  }

 protected:
  void on_response(int response_id) override;
  void on_show() override;

 private:
  void OnTaskNameChange() noexcept;
  std::string GetTrimmedEditText() noexcept;

  Gtk::Entry* edt_task_name_ = nullptr;
  Gtk::Button* btn_ok_ = nullptr;
  Gtk::CheckButton* chk_archived_ = nullptr;
  Task* task_ = nullptr;
};

}  // namespace m_time_tracker
