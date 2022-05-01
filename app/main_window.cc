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
#include "app/recent_activities_model.h"
#include "app/task.h"
#include "app/task_list_model_base.h"
#include "app/utils.h"

namespace m_time_tracker {

namespace {

class EditTaskListModel : public TaskListModelBase {
 public:
  EditTaskListModel(AppState* app_state, MainWindow* main_window) noexcept
      : TaskListModelBase(app_state, true),
        app_state_(app_state),
        main_window_(main_window) {
    VERIFY(main_window_);
    running_task_changed_connection_ = app_state_->ConnectRunningTaskChanged(
        sigc::mem_fun(*this, &EditTaskListModel::OnRunningTaskChanged));
    InitContent();
  }

  ~EditTaskListModel() {
    running_task_changed_connection_.disconnect();
  }

 protected:
  void CustomizeRow(
      const Glib::RefPtr<Gtk::ListBoxRow>, const Task&) noexcept override;
  void ReCustomizeRow(
      const Glib::RefPtr<Gtk::ListBoxRow>, const Task&) noexcept override;

 private:
  void EditTask(Task::Id task_id) {
    auto maybe_task = Task::LoadById(
        &app_state_->db_for_read_only(), task_id);
    VERIFY(maybe_task);
    main_window_->EditTask(&maybe_task.value());
  }

  void OnRunningTaskChanged(
      const std::optional<Task>& new_running_task) noexcept;

  AppState* const app_state_;
  MainWindow* const main_window_;
  sigc::connection running_task_changed_connection_;
  std::unordered_map<Task::Id, Glib::RefPtr<Gtk::Button>>
      task_id_to_btn_edit_;
};

class TaskListModel : public TaskListModelBase {
 public:
  explicit TaskListModel(AppState* app_state) noexcept
      : TaskListModelBase(app_state, false) {
    InitContent();
  }
};


void EditTaskListModel::CustomizeRow(
    const Glib::RefPtr<Gtk::ListBoxRow> wrapped_row, const Task& t) noexcept {
  Glib::RefPtr<Gtk::Button> btn_edit(new Gtk::Button());
  btn_edit->set_image_from_icon_name("gtk-edit");
  btn_edit->show();
  VERIFY(t.id());
  btn_edit->signal_clicked().connect([this, task_id = *t.id()]() {
    EditTask(task_id);
  });
  if (HDY_IS_ACTION_ROW(wrapped_row->gobj())) {
    hdy_action_row_add_prefix(
        reinterpret_cast<HdyActionRow*>(wrapped_row->gobj()),
        reinterpret_cast<GtkWidget*>(btn_edit->gobj()));
  } else if (HDY_IS_EXPANDER_ROW(wrapped_row->gobj())) {
    hdy_expander_row_add_prefix(
        reinterpret_cast<HdyExpanderRow*>(wrapped_row->gobj()),
        reinterpret_cast<GtkWidget*>(btn_edit->gobj()));
  } else {
    VERIFY(false);
  }

  task_id_to_btn_edit_[*t.id()] = btn_edit;

  const auto maybe_running_task = app_state_->running_task();
  if (maybe_running_task &&
      maybe_running_task->id() == t.id()) {
    btn_edit->set_sensitive(false);
  }
  ReCustomizeRow(wrapped_row, t);
}

void EditTaskListModel::ReCustomizeRow(
    const Glib::RefPtr<Gtk::ListBoxRow> wrapped_row, const Task& t) noexcept {
  Glib::RefPtr<Gtk::StyleContext> style_context =
      wrapped_row->get_style_context();
  if (t.is_archived()) {
    style_context->add_class("archived");
  } else {
    style_context->remove_class("archived");
  }
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

}  // namespace

MainWindow::MainWindow(
    GtkWindow* wnd,
    const Glib::RefPtr<Gtk::Builder>& builder,
    AppState* app_state)
    : Gtk::Window(wnd),
      resource_builder_(builder),
      app_state_(app_state),
      statistics_view_(this, builder, app_state) {
  VERIFY(app_state_);
  InitializeWidgetPointers(builder);
  page_stack_->property_visible_child().signal_changed().connect(
      sigc::mem_fun(*this, &MainWindow::OnPageStackVisibleChildChanged));

  btn_menu_->signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::OnBtnMenuClicked));
  btn_new_task_->signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::OnBtnNewTaskClicked));
  Glib::RefPtr<EditTaskListModel> edit_tasks_model(
      EditTaskListModel::create<EditTaskListModel>(app_state_, this));
  edit_tasks_model->BindTo(lst_edit_tasks_);
  task_list_model_ = TaskListModel::create<TaskListModel>(app_state_);
  task_list_model_->BindTo(lst_tasks_);
  task_list_model_->ConnectSelectedTaskIdChanged(
      sigc::mem_fun(*this, &MainWindow::OnLstTasksSelectionChanged));
  page_stack_sidebar_->signal_button_release_event().connect(
      sigc::mem_fun(*this, &MainWindow::OnStackSidebarButtonReleased));
  Glib::RefPtr<RecentActivitiesModel> activities_model =
      RecentActivitiesModel::create<RecentActivitiesModel>(
          app_state_, this, resource_builder_);
  lst_recent_activities_->bind_model(
      activities_model,
      activities_model->slot_create_widget());
  btn_start_stop_->signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::OnBtnStartStopClicked));
  btn_make_record_->signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::OnBtnMakeRecordClicked));
  running_task_changed_connection_ = app_state_->ConnectRunningTaskChanged(
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
  lst_recent_activities_ = GetWidgetChecked<Gtk::ListBox>(
      builder, "lst_recent_activities");
  btn_start_stop_ = GetWidgetChecked<Gtk::Button>(builder, "btn_start_stop");
  btn_make_record_ = GetWidgetChecked<Gtk::Button>(builder, "btn_make_record");
  box_statistics_ = GetWidgetChecked<Gtk::Box>(builder, "box_statistics");
}

void MainWindow::OnBtnMenuClicked() noexcept {
  if (main_stack_->get_visible_child() == page_stack_) {
    main_stack_->set_visible_child(*page_stack_sidebar_);
  } else {
    main_stack_->set_visible_child(*page_stack_);
  }
}

bool MainWindow::OnStackSidebarButtonReleased(GdkEventButton*) noexcept {
  main_stack_->set_visible_child(*page_stack_);
  return false;  // Allow event propagation.
}

void MainWindow::OnBtnNewTaskClicked() noexcept {
  Task new_task{std::string()};
  EditTask(&new_task);
}

void MainWindow::EditTask(Task* task) noexcept {
  Glib::RefPtr<EditTaskDialog> edit_task_dialog =
      GetWindowDerived<EditTaskDialog>(
          resource_builder_, "edit_task_dialog", app_state_);
  edit_task_dialog->set_task(task);
  while (true) {
    if (edit_task_dialog->run() != Gtk::RESPONSE_OK) {
      break;
    }
    // Check for already present task with that name.
    outcome::std_result<Task> maybe_conflicting_task = Task::LoadByName(
        &app_state_->db_for_read_only(),
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

    const auto save_result = app_state_->SaveTask(task);
    // TODO(vchigrin): Better error handling.
    VERIFY(save_result);
    break;
  }
  // Ensure we'll never produce dangling pointers. Note, that dialog object
  // may re reused since Gtk::Builder holds reference to it.
  edit_task_dialog->set_task(nullptr);
  edit_task_dialog->hide();
}

void MainWindow::OnPageStackVisibleChildChanged() noexcept {
  main_stack_->set_visible_child(*page_stack_);
  if (page_stack_->get_visible_child() == box_statistics_) {
    statistics_view_.Recalculate();
  }
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
      const auto save_result = app_state_->RecordRunningTaskActivity();
      // TODO(vchigrin): Better error handling.
      VERIFY(save_result);
    }
    app_state_->DropRunningTask();
  } else {
    timer_connection_ = Glib::signal_timeout().connect_seconds(
        sigc::mem_fun(*this, &MainWindow::OnTaskTimer), 1);
    Gtk::ListBoxRow* selected_row = lst_tasks_->get_selected_row();
    VERIFY(selected_row);  // Button should be disabled if selection is abent.
    const Task::Id task_id = task_list_model_->GetTaskIdForRow(selected_row);
    auto maybe_task = Task::LoadById(&app_state_->db_for_read_only(), task_id);
    VERIFY(maybe_task);
    app_state_->StartRunningTask(maybe_task.value());
  }
}

void MainWindow::OnBtnMakeRecordClicked() noexcept {
  VERIFY(IsTaskRunning());
  const auto save_result = app_state_->RecordRunningTaskActivity();
  // TODO(vchigrin): Better error handling.
  VERIFY(save_result);
}

void MainWindow::OnLstTasksSelectionChanged(
    const std::optional<Task::Id>& selected_task_id) noexcept {
  if (IsTaskRunning()) {
    // Active task archiving or changing must be disallowed by UI.
    VERIFY(selected_task_id);
    auto maybe_task = Task::LoadById(
        &app_state_->db_for_read_only(),
        *selected_task_id);
    VERIFY(maybe_task);
    app_state_->ChangeRunningTask(std::move(maybe_task.value()));
  }
  UpdateBtnStartStop();
}

void MainWindow::UpdateBtnStartStop() noexcept {
  const std::optional<Task::Id> selected_task_id =
      task_list_model_->selected_task_id();
  if (selected_task_id) {
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
  std::optional<Task> running_task = app_state_->running_task();
  if (!running_task) {
    // TODO(vchigrin): Localization.
    lbl_running_time_->set_text("<No task running>");
  } else {
    auto maybe_runtime = app_state_->RunningTaskRunTime();
    VERIFY(maybe_runtime);
    const Activity::Duration runtime = *maybe_runtime;
    // TODO(vchigrin): Localization.
    lbl_running_time_->set_text((boost::format("Running: %1% for %2%") %
        running_task->name() %
        FormatRuntime(runtime, FormatMode::kShortWithSeconds)).str());
  }
}

}  // namespace m_time_tracker
