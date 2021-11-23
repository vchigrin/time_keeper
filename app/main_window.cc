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
#include "app/task_list_model_base.h"
#include "app/task.h"

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
  }

  Glib::RefPtr<Gtk::Widget> CreateRowFromTask(const Task& t) noexcept override;

 private:
  void EditTask(int64_t task_id) {
    auto maybe_task = Task::LoadById(
        &db_wrapper_->db_for_read_only(), task_id);
    VERIFY(maybe_task);
    main_window_->EditTask(&maybe_task.value());
  }
  DbWrapper* const db_wrapper_;
  MainWindow* const main_window_;
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
  Gtk::Button* btn_edit = manage(new Gtk::Button());
  btn_edit->set_image_from_icon_name("gtk-edit");
  btn_edit->show();
  VERIFY(t.id());
  btn_edit->signal_clicked().connect([this, task_id = *t.id()]() {
    EditTask(task_id);
  });
  wrapped_row->add(*btn_edit);

  if (t.is_archived()) {
    Glib::RefPtr<Gtk::StyleContext> style_context =
        wrapped_row->get_style_context();
    style_context->add_class("archived");
  }

  wrapped_row->show();
  return wrapped_row;
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
  UpdateBtnStartStop(false);
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
  lst_edit_tasks_ = GetWidgetChecked<Gtk::ListBox>(
      builder, "lst_edit_tasks");
  lst_tasks_ = GetWidgetChecked<Gtk::ListBox>(builder, "lst_tasks");
  btn_start_stop_ = GetWidgetChecked<Gtk::Button>(builder, "btn_start_stop");
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
  is_task_running_ = !is_task_running_;
  Gtk::Image* child = dynamic_cast<Gtk::Image*>(btn_start_stop_->get_child());
  VERIFY(child);
  if (is_task_running_) {
    child->property_icon_name() = "media-playback-stop-symbolic";
  } else {
    child->property_icon_name() = "media-playback-start-symbolic";
  }
}

void MainWindow::OnLstTasksRowSelected(
    Gtk::ListBoxRow* selected_row) noexcept {
  std::optional<int64_t> task_id = std::nullopt;
  if (selected_row) {
    task_id = task_list_model_->FindTaskIdForRow(selected_row);
    VERIFY(task_id);
    auto maybe_task = Task::LoadById(
        &db_wrapper_->db_for_read_only(), *task_id);
    VERIFY(maybe_task);
    lbl_running_time_->set_text(maybe_task.value().name());
  } else {
    // TODO(vchigrin): Disallow archiving or changing running tasks.
    lbl_running_time_->set_text("None");
  }
  UpdateBtnStartStop(task_id != std::nullopt);
}

void MainWindow::UpdateBtnStartStop(bool task_selected) noexcept {
  if (task_selected) {
    btn_start_stop_->set_sensitive(true);
  } else {
    // TODO(vchigrin): Disallow archiving or changing running tasks.
    if (!is_task_running_) {
      btn_start_stop_->set_sensitive(false);
    }
  }
}

}  // namespace m_time_tracker
