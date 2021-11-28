// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/main_window.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/format.hpp>

#include "app/activity.h"
#include "app/edit_task_dialog.h"
#include "app/task_list_model_base.h"
#include "app/task.h"
#include "app/utils.h"

namespace m_time_tracker {

namespace {

class EditTaskListModel : public TaskListModelBase {
 protected:
  friend class TaskListModelBase;
  EditTaskListModel(DbWrapper* db_wrapper, MainWindow* main_window) noexcept
      : TaskListModelBase(db_wrapper),
        db_wrapper_(db_wrapper),
        main_window_(main_window) {
    auto maybe_all_tasks = Task::LoadAll(&db_wrapper->db_for_read_only());
    VERIFY(maybe_all_tasks);
    Initialize(maybe_all_tasks.value());
    VERIFY(main_window_);
    running_task_changed_connection_ = db_wrapper_->ConnectRunningTaskChanged(
        sigc::mem_fun(*this, &EditTaskListModel::OnRunningTaskChanged));
  }

  ~EditTaskListModel() {
    running_task_changed_connection_.disconnect();
  }

  Glib::RefPtr<Gtk::Widget> CreateRowFromTask(const Task& t) noexcept override;

 private:
  void EditTask(Task::Id task_id) {
    auto maybe_task = Task::LoadById(
        &db_wrapper_->db_for_read_only(), task_id);
    VERIFY(maybe_task);
    main_window_->EditTask(&maybe_task.value());
  }

  void OnRunningTaskChanged(
      const std::optional<Task>& new_running_task) noexcept;

  DbWrapper* const db_wrapper_;
  MainWindow* const main_window_;
  sigc::connection running_task_changed_connection_;
  std::unordered_map<Task::Id, Glib::RefPtr<Gtk::Button>>
      task_id_to_btn_edit_;
};

class TaskListModel : public TaskListModelBase {
 protected:
  friend class TaskListModelBase;
  explicit TaskListModel(DbWrapper* db_wrapper) noexcept
      : TaskListModelBase(db_wrapper) {
    auto maybe_tasks = Task::LoadNotArchived(
        &db_wrapper->db_for_read_only());
    VERIFY(maybe_tasks);
    Initialize(maybe_tasks.value());
  }

  Glib::RefPtr<Gtk::Widget> CreateRowFromTask(const Task& t) noexcept override;
};

Glib::RefPtr<Gtk::Widget> EditTaskListModel::CreateRowFromTask(
    const Task& t) noexcept {
  GtkListBoxRow* row = reinterpret_cast<GtkListBoxRow*>(hdy_action_row_new());
  g_object_ref_sink(row);
  Glib::RefPtr<Gtk::ListBoxRow> wrapped_row(Glib::wrap(row));

  const std::string& title = t.name();
  hdy_preferences_row_set_title(
      reinterpret_cast<HdyPreferencesRow*>(wrapped_row->gobj()),
      title.c_str());
  Glib::RefPtr<Gtk::Button> btn_edit(new Gtk::Button());
  btn_edit->set_image_from_icon_name("gtk-edit");
  btn_edit->show();
  VERIFY(t.id());
  btn_edit->signal_clicked().connect([this, task_id = *t.id()]() {
    EditTask(task_id);
  });
  wrapped_row->add(*btn_edit.get());

  if (t.is_archived()) {
    Glib::RefPtr<Gtk::StyleContext> style_context =
        wrapped_row->get_style_context();
    style_context->add_class("archived");
  }
  task_id_to_btn_edit_[*t.id()] = btn_edit;

  const auto maybe_running_task = db_wrapper_->running_task();
  if (maybe_running_task &&
      maybe_running_task->id() == t.id()) {
    btn_edit->set_sensitive(false);
  }

  wrapped_row->show();
  return wrapped_row;
}

void EditTaskListModel::OnRunningTaskChanged(
    const std::optional<Task>& new_running_task) noexcept {
  for (const auto [task_id, btn_edit] : task_id_to_btn_edit_) {
    if (new_running_task && new_running_task->id() == task_id) {
      btn_edit->set_sensitive(false);
    } else {
      btn_edit->set_sensitive(true);
    }
  }
}

Glib::RefPtr<Gtk::Widget> TaskListModel::CreateRowFromTask(
    const Task& t) noexcept {
  if (t.is_archived()) {
    // May be if task was changed.
    return Glib::RefPtr<Gtk::Widget>();
  }
  GtkListBoxRow* row = reinterpret_cast<GtkListBoxRow*>(hdy_action_row_new());
  g_object_ref_sink(row);
  Glib::RefPtr<Gtk::ListBoxRow> wrapped_row(Glib::wrap(row));

  const std::string& title = t.name();
  hdy_preferences_row_set_title(
      reinterpret_cast<HdyPreferencesRow*>(wrapped_row->gobj()),
      title.c_str());
  wrapped_row->show();
  return wrapped_row;
}

}  // namespace

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
  Glib::RefPtr<EditTaskListModel> edit_tasks_model =
      TaskListModelBase::create<EditTaskListModel>(db_wrapper_, this);
  lst_edit_tasks_->bind_model(
      edit_tasks_model,
      edit_tasks_model->slot_create_widget());
  task_list_model_ = TaskListModelBase::create<TaskListModel>(db_wrapper_);
  lst_tasks_->bind_model(
      task_list_model_,
      task_list_model_->slot_create_widget());
  lst_tasks_->signal_row_selected().connect(
      sigc::mem_fun(*this, &MainWindow::OnLstTasksRowSelected));
  btn_start_stop_->signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::OnBtnStartStopClicked));
  btn_make_record_->signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::OnBtnMakeRecordClicked));
  running_task_changed_connection_ = db_wrapper_->ConnectRunningTaskChanged(
      sigc::mem_fun(*this, &MainWindow::OnRunningTaskChanged));
  OnRunningTaskChanged(std::nullopt);
}

MainWindow::~MainWindow() {
  timer_connection_.disconnect();
  running_task_changed_connection_.disconnect();
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
  lst_tasks_ = GetWidgetChecked<Gtk::ListBox>(builder, "lst_tasks");
  btn_start_stop_ = GetWidgetChecked<Gtk::Button>(builder, "btn_start_stop");
  btn_make_record_ = GetWidgetChecked<Gtk::Button>(builder, "btn_make_record");
}

void MainWindow::OnBtnMenuClicked() noexcept {
  if (main_stack_->get_visible_child() == page_stack_) {
    main_stack_->set_visible_child(*page_stack_sidebar_);
  } else {
    main_stack_->set_visible_child(*page_stack_);
  }
}

void MainWindow::OnBtnNewTaskClicked() noexcept {
  Task new_task{std::string()};
  EditTask(&new_task);
}

void MainWindow::EditTask(Task* task) noexcept {
  if (!edit_task_dialog_) {
    // Create dialog just once, then re-use it. If we destoy it GtkBuilder
    // will still attempt to return reference to the old object.
    edit_task_dialog_ =
      GetWindowDerived<EditTaskDialog>(resource_builder_, "edit_task_dialog");
  }
  edit_task_dialog_->set_task(task);
  while (true) {
    if (edit_task_dialog_->run() != Gtk::RESPONSE_OK) {
      break;
    }
    // Check for already present task with that name.
    outcome::std_result<Task> maybe_conflicting_task = Task::LoadByName(
        &db_wrapper_->db_for_read_only(),
        task->name());
    if (maybe_conflicting_task &&
        maybe_conflicting_task.value().id() != task->id()) {
      Gtk::MessageDialog message_dlg(
          *this,
          // TODO(vchigrin): Localization.
          "Error - this name already used by another task",
          /* use_markup */ false,
          Gtk::MESSAGE_ERROR,
          Gtk::BUTTONS_OK,
          /* modal */ true);
      message_dlg.run();
      continue;
    }

    const auto save_result = db_wrapper_->SaveTask(task);
    // TODO(vchigrin): Better error handling.
    VERIFY(save_result);
    break;
  }
  // Ensure we'll never produce dangling pointers.
  edit_task_dialog_->set_task(nullptr);
  edit_task_dialog_->hide();
}

void MainWindow::OnPageStackVisibleChildChanged() noexcept {
  main_stack_->set_visible_child(*page_stack_);
}

void MainWindow::OnBtnStartStopClicked() noexcept {
  if (IsTaskRunning()) {
    timer_connection_.disconnect();
    Gtk::MessageDialog message_dlg(
        *this,
        // TODO(vchigrin): Localization.
        "Do you want make record about running task before stopping?",
        /* use_markup */ false,
        Gtk::MESSAGE_QUESTION,
        Gtk::BUTTONS_YES_NO,
        /* modal */ true);
    const int result = message_dlg.run();
    if (result == Gtk::RESPONSE_YES) {
      const auto save_result = db_wrapper_->RecordRunningTaskActivity();
      // TODO(vchigrin): Better error handling.
      VERIFY(save_result);
    }
    db_wrapper_->DropRunningTask();
  } else {
    timer_connection_ = Glib::signal_timeout().connect_seconds(
        sigc::mem_fun(*this, &MainWindow::OnTaskTimer), 1);
    Gtk::ListBoxRow* selected_row = lst_tasks_->get_selected_row();
    VERIFY(selected_row);  // Button should be disabled if selection is abent.
    const auto task_id = task_list_model_->FindTaskIdForRow(selected_row);
    VERIFY(task_id);
    auto maybe_task = Task::LoadById(
        &db_wrapper_->db_for_read_only(), *task_id);
    VERIFY(maybe_task);
    db_wrapper_->StartRunningTask(maybe_task.value());
  }
}

void MainWindow::OnBtnMakeRecordClicked() noexcept {
  VERIFY(IsTaskRunning());
  const auto save_result = db_wrapper_->RecordRunningTaskActivity();
  // TODO(vchigrin): Better error handling.
  VERIFY(save_result);
}

void MainWindow::OnLstTasksRowSelected(
    Gtk::ListBoxRow* selected_row) noexcept {
  std::optional<Task> task = std::nullopt;
  if (selected_row) {
    const std::optional<Task::Id> task_id =
        task_list_model_->FindTaskIdForRow(selected_row);
    VERIFY(task_id);
    auto maybe_task = Task::LoadById(
        &db_wrapper_->db_for_read_only(), *task_id);
    VERIFY(maybe_task);
    task = std::move(maybe_task.value());
  } else {
    // Active task archiving or changing must be disallowed by UI.
    VERIFY(!IsTaskRunning());
  }
  if (IsTaskRunning()) {
    VERIFY(task);
    db_wrapper_->ChangeRunningTask(std::move(*task));
  }
  UpdateBtnStartStop();
}

void MainWindow::UpdateBtnStartStop() noexcept {
  const Gtk::ListBoxRow* selected_row = lst_tasks_->get_selected_row();
  if (selected_row) {
    btn_start_stop_->set_sensitive(true);
  } else {
    btn_start_stop_->set_sensitive(false);
    // Active task archiving or changing must be disallowed by UI.
    VERIFY(!IsTaskRunning());
  }
  Gtk::Image* child = dynamic_cast<Gtk::Image*>(btn_start_stop_->get_child());
  VERIFY(child);
  if (IsTaskRunning()) {
    child->property_icon_name() = "media-playback-stop-symbolic";
  } else {
    child->property_icon_name() = "media-playback-start-symbolic";
  }
}

bool MainWindow::OnTaskTimer() noexcept {
  VERIFY(IsTaskRunning());
  UpdateLblRunningTime();
  return true;  // Continue firing timer events.
}

void MainWindow::OnRunningTaskChanged(
    const std::optional<Task>& running_task) noexcept {
  UpdateLblRunningTime();
  UpdateBtnStartStop();
  btn_make_record_->set_sensitive(running_task != std::nullopt);
}

void MainWindow::UpdateLblRunningTime() noexcept {
  std::optional<Task> running_task = db_wrapper_->running_task();
  if (!running_task) {
    // TODO(vchigrin): Localization.
    lbl_running_time_->set_text("<No task running>");
  } else {
    auto maybe_runtime = db_wrapper_->RunningTaskRunTime();
    VERIFY(maybe_runtime);
    const Activity::Duration runtime = *maybe_runtime;
    // TODO(vchigrin): Localization.
    lbl_running_time_->set_text((boost::format("Running: %1% for %2%")
        % running_task->name() % FormatRuntime(runtime)).str());
  }
}

}  // namespace m_time_tracker
