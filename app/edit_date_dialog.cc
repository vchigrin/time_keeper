// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/edit_date_dialog.h"

namespace m_time_tracker {

EditDateDialog::EditDateDialog(
    GtkDialog* dlg, const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::Dialog(dlg) {
  cal_date_ = GetWidgetChecked<Gtk::Calendar>(builder, "cal_date");
}

void EditDateDialog::on_response(int response_id) {
  if (response_id == Gtk::RESPONSE_OK) {
    guint year = 0;
    guint month = 0;
    guint day = 0;
    cal_date_-> get_date(year, month, day);
    date_.tm_year = year - 1900;
    date_.tm_mon = month;
    date_.tm_mday = day;
  }
}

void EditDateDialog::on_show() {
  Gtk::Dialog::on_show();
  cal_date_->select_month(date_.tm_mon, date_.tm_year + 1900);
  cal_date_->select_day(date_.tm_mday);
}

}  // namespace m_time_tracker
