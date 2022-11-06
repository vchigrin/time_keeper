// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <ctime>
#include <string>
#include <utility>

#include "app/ui_helpers.h"
#include "app/task.h"

namespace m_time_tracker {

class EditDateDialog : public Gtk::Dialog {
 public:
  EditDateDialog(GtkDialog* dlg, const Glib::RefPtr<Gtk::Builder>& builder);

  // Time fields are not affected by this object.
  void set_date(const std::tm& localtime) noexcept {
    date_ = localtime;
  }

  const std::tm& get_date() noexcept {
    return date_;
  }

 protected:
  void on_response(int response_id) override;
  void on_show() override;

 private:
  Gtk::Calendar* cal_date_ = nullptr;
  std::tm date_;
};

}  // namespace m_time_tracker
