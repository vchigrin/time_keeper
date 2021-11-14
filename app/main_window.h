// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include "app/ui_helpers.h"
namespace m_time_tracker {


class MainWindow : public Gtk::Window {
 public:
  MainWindow(GtkWindow* wnd, const Glib::RefPtr<Gtk::Builder>& builder);

 private:
  void InitializeWidgetPointers(
      const Glib::RefPtr<Gtk::Builder>& builder) noexcept;

  void OnBtnMenuClicked() noexcept;
  void OnPageStackVisibleChildChanged() noexcept;

  Gtk::Button* btn_menu_ = nullptr;
  Gtk::Stack* main_stack_ = nullptr;
  Gtk::Stack* page_stack_ = nullptr;
  Gtk::Label* lbl_running_time_ = nullptr;
  Gtk::StackSidebar* page_stack_sidebar_ = nullptr;
};

}  // namespace m_time_tracker
