// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <optional>

#include "app/activity.h"
#include "app/ui_helpers.h"
#include "app/app_state.h"
#include "app/statistics_view.h"

namespace m_time_tracker {

class EditTaskDialog;
class TaskListModelBase;

class MainWindow : public Gtk::Window {
 public:
  // AppState must outlive this object.
  MainWindow(
      GtkWindow* wnd,
      const Glib::RefPtr<Gtk::Builder>& builder,
      AppState* app_state);
  ~MainWindow();

  void EditTask(Task* task) noexcept;

  [[noreturn]]
  void OnFatalError(const std::error_code& ec) noexcept;

 private:
  void InitializeWidgetPointers(
      const Glib::RefPtr<Gtk::Builder>& builder) noexcept;
  void StartTaskTimer() noexcept;

  void OnBtnMenuClicked() noexcept;
  bool OnStackSidebarButtonReleased(GdkEventButton*) noexcept;
  void OnBtnNewTaskClicked() noexcept;
  void OnPageStackVisibleChildChanged() noexcept;
  void RefreshTasksList() noexcept;

  void OnBtnStartStopClicked() noexcept;
  void OnBtnMakeRecordClicked() noexcept;
  void OnLstTasksSelectionChanged(
      const std::optional<Task::Id>& selected_task_id) noexcept;
  void OnRunningTaskChanged(const std::optional<Task>&) noexcept;
  void UpdateBtnStartStop() noexcept;
  bool OnTaskTimer() noexcept;
  void UpdateLblRunningTime() noexcept;
  bool IsTaskRunning() const noexcept {
    return app_state_->running_task() != std::nullopt;
  }

  Gtk::Button* btn_menu_ = nullptr;
  Gtk::Button* btn_new_task_ = nullptr;
  Gtk::Button* btn_start_stop_ = nullptr;
  Gtk::Button* btn_make_record_ = nullptr;
  Gtk::Stack* main_stack_ = nullptr;
  Gtk::Stack* page_stack_ = nullptr;
  Gtk::Label* lbl_running_time_ = nullptr;
  Gtk::ListBox* lst_edit_tasks_ = nullptr;
  Gtk::ListBox* lst_tasks_ = nullptr;
  Gtk::ListBox* lst_recent_activities_ = nullptr;
  Gtk::Box* box_statistics_ = nullptr;
  Gtk::StackSidebar* page_stack_sidebar_ = nullptr;
  Glib::RefPtr<Gtk::Builder> resource_builder_;
  AppState* const app_state_;
  sigc::connection running_task_changed_connection_;
  Glib::RefPtr<TaskListModelBase> task_list_model_;
  // Timer is active only when task is running.
  sigc::connection timer_connection_;
  StatisticsView statistics_view_;
};

}  // namespace m_time_tracker
