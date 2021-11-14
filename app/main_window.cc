// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/main_window.h"

namespace m_time_tracker {

MainWindow::MainWindow(
    GtkWindow* wnd, const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::Window(wnd) {
  InitializeWidgetPointers(builder);
  page_stack_->property_visible_child().signal_changed().connect(
      sigc::mem_fun(*this, &MainWindow::OnPageStackVisibleChildChanged));

  btn_menu_->signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::OnBtnMenuClicked));
}

void MainWindow::InitializeWidgetPointers(
    const Glib::RefPtr<Gtk::Builder>& builder) noexcept {
  btn_menu_ = GetWidgetChecked<Gtk::Button>(builder, "btn_menu");
  main_stack_ = GetWidgetChecked<Gtk::Stack>(builder, "main_stack");
  page_stack_ = GetWidgetChecked<Gtk::Stack>(builder, "page_stack");
  page_stack_sidebar_ = GetWidgetChecked<Gtk::StackSidebar>(
      builder, "page_stack_sidebar");
  lbl_running_time_ = GetWidgetChecked<Gtk::Label>(
      builder, "lbl_running_time");
}

void MainWindow::OnBtnMenuClicked() noexcept {
  main_stack_->set_visible_child(*page_stack_sidebar_);
}

void MainWindow::OnPageStackVisibleChildChanged() noexcept {
  main_stack_->set_visible_child(*page_stack_);
}

}  // namespace m_time_tracker
