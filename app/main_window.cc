// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/main_window.h"

#include <string>
#include <utility>

#include "app/edit_task_dialog.h"
#include "app/task.h"

namespace m_time_tracker {

MainWindow::MainWindow(
    GtkWindow* wnd, const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::Window(wnd),
      resource_builder_(builder) {
  InitializeWidgetPointers(builder);
  page_stack_->property_visible_child().signal_changed().connect(
      sigc::mem_fun(*this, &MainWindow::OnPageStackVisibleChildChanged));

  btn_menu_->signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::OnBtnMenuClicked));
  btn_new_task_->signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::OnBtnNewTaskClicked));

  // TODO(vchigrin): Save DB in permanent location.
  outcome::std_result<Database> maybe_db = Database::Open(":memory:");
  VERIFY(maybe_db);
  db_.emplace(std::move(maybe_db.value()));
  // TODO(vchigrin): This must NOT be in UI code..
  const auto init_result = Task::EnsureTableCreated(&*db_);
  VERIFY(init_result);
  RefreshTasksList();
}

MainWindow::~MainWindow() = default;

void MainWindow::InitializeWidgetPointers(
    const Glib::RefPtr<Gtk::Builder>& builder) noexcept {
  btn_menu_ = GetWidgetChecked<Gtk::Button>(builder, "btn_menu");
  btn_new_task_ = GetWidgetChecked<Gtk::Button>(builder, "btn_new_task");
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

void MainWindow::OnBtnNewTaskClicked() noexcept {
  if (!edit_task_dialog_) {
    // Create dialog just once, then re-use it. If we destoy it GtkBuilder
    // will still attempt to return reference to the old object.
    edit_task_dialog_ =
      GetWindowDerived<EditTaskDialog>(resource_builder_, "edit_task_dialog");
  }
  edit_task_dialog_->set_task_name(std::string());
  if (edit_task_dialog_->run() == Gtk::RESPONSE_OK) {
    std::string task_name = edit_task_dialog_->task_name();
    Task new_task(std::move(task_name));
    VERIFY(db_);
    const auto save_result = new_task.Save(&*db_);
    // TODO(vchigrin): Better error handling.
    VERIFY(save_result);
    RefreshTasksList();
  }
  edit_task_dialog_->hide();
}

void MainWindow::OnPageStackVisibleChildChanged() noexcept {
  main_stack_->set_visible_child(*page_stack_);
}

void MainWindow::RefreshTasksList() noexcept {
}

}  // namespace m_time_tracker
