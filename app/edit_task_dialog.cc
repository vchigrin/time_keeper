// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/edit_task_dialog.h"

namespace m_time_tracker {

EditTaskDialog::EditTaskDialog(
    GtkDialog* dlg, const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::Dialog(dlg) {
  edt_task_name_ = GetWidgetChecked<Gtk::Entry>(builder, "edt_task_name");
}

void EditTaskDialog::on_response(int response_id) {
  if (response_id == Gtk::RESPONSE_OK) {
    task_name_ = edt_task_name_->get_text();
  }
}

void EditTaskDialog::on_show() {
  Gtk::Dialog::on_show();
  edt_task_name_->set_text(task_name_);
}

}  // namespace m_time_tracker
