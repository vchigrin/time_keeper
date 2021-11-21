// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/edit_task_dialog.h"

#include <boost/algorithm/string/trim.hpp>

namespace m_time_tracker {

EditTaskDialog::EditTaskDialog(
    GtkDialog* dlg, const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::Dialog(dlg) {
  edt_task_name_ = GetWidgetChecked<Gtk::Entry>(builder, "edt_task_name");
  btn_ok_ = GetWidgetChecked<Gtk::Button>(builder, "btn_ok");
  edt_task_name_->signal_changed().connect(
      sigc::mem_fun(*this, &EditTaskDialog::OnTaskNameChange));
}

void EditTaskDialog::on_response(int response_id) {
  if (response_id == Gtk::RESPONSE_OK) {
    task_name_ = GetTrimmedEditText();
  }
}

void EditTaskDialog::on_show() {
  Gtk::Dialog::on_show();
  edt_task_name_->set_text(task_name_);
  OnTaskNameChange();
}

std::string EditTaskDialog::GetTrimmedEditText() noexcept {
  std::string text = edt_task_name_->get_text();
  return boost::algorithm::trim_copy(text);
}

void EditTaskDialog::OnTaskNameChange() noexcept {
  const std::string text = GetTrimmedEditText();
  btn_ok_->set_sensitive(!text.empty());
}

}  // namespace m_time_tracker
