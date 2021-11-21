// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <utility>

#include "app/ui_helpers.h"

namespace m_time_tracker {

class EditTaskDialog : public Gtk::Dialog {
 public:
  EditTaskDialog(GtkDialog* dlg, const Glib::RefPtr<Gtk::Builder>& builder);

  const std::string& task_name() const noexcept {
    return task_name_;
  }

  void set_task_name(std::string new_name) noexcept {
    task_name_ = std::move(new_name);
  }

 protected:
  void on_response(int response_id) override;
  void on_show() override;

 private:
  void OnTaskNameChange() noexcept;
  std::string GetTrimmedEditText() noexcept;

  Gtk::Entry* edt_task_name_ = nullptr;
  Gtk::Button* btn_ok_ = nullptr;
  std::string task_name_;
};

}  // namespace m_time_tracker
