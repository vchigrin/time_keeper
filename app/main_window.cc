// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/main_window.h"

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "app/activity.h"
#include "app/edit_task_dialog.h"
#include "app/task.h"

namespace m_time_tracker {

MainWindow::MainWindow(
    GtkWindow* wnd,
    const Glib::RefPtr<Gtk::Builder>& builder,
    DbWrapper* db_wrapper)
    : Gtk::Window(wnd),
      resource_builder_(builder),
      db_wrapper_(db_wrapper) {
  VERIFY(db_wrapper_);
  InitializeWidgetPointers(builder);
  page_stack_->property_visible_child().signal_changed().connect(
      sigc::mem_fun(*this, &MainWindow::OnPageStackVisibleChildChanged));

  btn_menu_->signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::OnBtnMenuClicked));
  btn_new_task_->signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::OnBtnNewTaskClicked));

  RefreshTasksList();
  task_list_changed_connection_ = db_wrapper_->ConnectToTaskListChanged(
      [this]() {
        RefreshTasksList();
      });
}

MainWindow::~MainWindow() {
  task_list_changed_connection_.disconnect();
}

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
  lst_edit_tasks_ = GetWidgetChecked<Gtk::ListBox>(
      builder, "lst_edit_tasks");
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
    const auto save_result = db_wrapper_->SaveTask(&new_task);
    // TODO(vchigrin): Better error handling.
    VERIFY(save_result);
  }
  edit_task_dialog_->hide();
}

void MainWindow::OnPageStackVisibleChildChanged() noexcept {
  main_stack_->set_visible_child(*page_stack_);
}

void MainWindow::RefreshTasksList() noexcept {
  auto maybe_all_tasks = Task::LoadAll(
      &db_wrapper_->db_for_read_only());
  VERIFY(maybe_all_tasks);
  const std::vector<Task>& tasks = maybe_all_tasks.value();
  for (const Task& t : tasks) {
    VERIFY(t.id());
    const int64_t task_id = *t.id();
    if (auto it = task_id_to_lst_tasks_items_.find(task_id);
        it != task_id_to_lst_tasks_items_.end()) {
      FillListRowFromTask(t, it->second.get());
    } else {
      std::unique_ptr<Gtk::Widget> wrapped_row(
          Glib::wrap(hdy_action_row_new()));
      wrapped_row->show();
      FillListRowFromTask(t, wrapped_row.get());
      lst_edit_tasks_->add(*wrapped_row);
      task_id_to_lst_tasks_items_.insert({task_id, std::move(wrapped_row)});
    }
  }
  std::unordered_set<int64_t> actual_task_ids;
  for (const Task& t : tasks) {
    VERIFY(actual_task_ids.insert(*t.id()).second);
  }
  for (auto it = task_id_to_lst_tasks_items_.begin();
      it != task_id_to_lst_tasks_items_.end(); ) {
    if (actual_task_ids.count(it->first)) {
      ++it;
    } else {
      it = task_id_to_lst_tasks_items_.erase(it);
    }
  }
}

void MainWindow::FillListRowFromTask(
    const Task& t,
    Gtk::Widget* row_widget) noexcept {
  const std::string& title = t.name();
  hdy_preferences_row_set_title(
      reinterpret_cast<HdyPreferencesRow*>(row_widget->gobj()), title.c_str());
}

}  // namespace m_time_tracker
