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
  chk_archived_ = GetWidgetChecked<Gtk::CheckButton>(builder, "chk_archived");
  edt_task_name_->signal_changed().connect(
      sigc::mem_fun(*this, &EditTaskDialog::OnTaskNameChange));
}

void EditTaskDialog::on_response(int response_id) {
  if (response_id == Gtk::RESPONSE_OK && task_) {
    task_->set_name(GetTrimmedEditText());
    task_->set_archived(chk_archived_->get_active());
  }
}

void EditTaskDialog::on_show() {
  Gtk::Dialog::on_show();
  if (task_) {
    edt_task_name_->set_text(task_->name());
    chk_archived_->set_active(task_->is_archived());
  }
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
