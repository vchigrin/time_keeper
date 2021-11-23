// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "app/ui_helpers.h"
#include "app/db_wrapper.h"

namespace m_time_tracker {

class EditTaskDialog;
class TaskListModelBase;

class MainWindow : public Gtk::Window {
 public:
  // DbWrapper must outlive this object.
  MainWindow(
      GtkWindow* wnd,
      const Glib::RefPtr<Gtk::Builder>& builder,
      DbWrapper* db_wrapper);
  ~MainWindow();

  void EditTask(Task* task) noexcept;

 private:
  void InitializeWidgetPointers(
      const Glib::RefPtr<Gtk::Builder>& builder) noexcept;

  void OnBtnMenuClicked() noexcept;
  void OnBtnNewTaskClicked() noexcept;
  void OnPageStackVisibleChildChanged() noexcept;
  void RefreshTasksList() noexcept;

  void OnBtnStartStopClicked() noexcept;
  void OnLstTasksRowSelected(Gtk::ListBoxRow* selected_row) noexcept;
  void UpdateBtnStartStop(bool task_selected) noexcept;

  Gtk::Button* btn_menu_ = nullptr;
  Gtk::Button* btn_new_task_ = nullptr;
  Gtk::Button* btn_start_stop_ = nullptr;
  Gtk::Stack* main_stack_ = nullptr;
  Gtk::Stack* page_stack_ = nullptr;
  Gtk::Label* lbl_running_time_ = nullptr;
  Gtk::ListBox* lst_edit_tasks_ = nullptr;
  Gtk::ListBox* lst_tasks_ = nullptr;
  Gtk::StackSidebar* page_stack_sidebar_ = nullptr;
  Glib::RefPtr<Gtk::Builder> resource_builder_;
  DbWrapper* const db_wrapper_;
  std::unique_ptr<EditTaskDialog> edit_task_dialog_;
  sigc::connection task_list_changed_connection_;
  Glib::RefPtr<TaskListModelBase> task_list_model_;
  bool is_task_running_ = false;
};

}  // namespace m_time_tracker
