// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/task_list_model_base.h"

#include <string>
#include <utility>

#include "app/app_state.h"
#include "app/list_model_base.h"
#include "app/task.h"

namespace m_time_tracker {

namespace {

template<typename T>
class ScopedChangeValue {
 public:
  ScopedChangeValue(T* ptr, T new_val) noexcept
      : ptr_(ptr),
        old_value_(std::move(*ptr)) {
    *ptr_ = std::move(new_val);
  }

  ~ScopedChangeValue() {
    *ptr_ = std::move(old_value_);
  }

 private:
  T* ptr_;
  T old_value_;
};
}  // namespace


class ChildTaskListModel: public ListModelBase<Task> {
 public:
  using ListModelBase::SetContent;
  friend class ListModelBase<Task>;

 protected:
  ChildTaskListModel(
      AppState* app_state,
      Task::Id parent_task_id,
      bool should_display_archived,
      TaskListModelBase* parent_model) noexcept
      : ListModelBase(app_state),
        parent_task_id_(parent_task_id),
        should_display_archived_(should_display_archived),
        parent_model_(parent_model) {
    all_connections_.emplace_back(app_state->ConnectExistingTaskChanged(
        sigc::mem_fun(*this, &ChildTaskListModel::ExistingObjectChanged)));
    all_connections_.emplace_back(app_state->ConnectAfterTaskAdded(
        sigc::mem_fun(*this, &ChildTaskListModel::AfterObjectAdded)));
    all_connections_.emplace_back(app_state->ConnectBeforeTaskDeleted(
        sigc::mem_fun(*this, &ChildTaskListModel::BeforeObjectDeleted)));
  }

  Glib::RefPtr<Gtk::Widget> CreateRowFromObject(
      const Task& t) noexcept override;

 private:
  const Task::Id parent_task_id_;
  const bool should_display_archived_;
  TaskListModelBase* const parent_model_;
};

Glib::RefPtr<Gtk::Widget> ChildTaskListModel::CreateRowFromObject(
    const Task& t) noexcept {
  if (!t.parent_task_id() ||
      *t.parent_task_id() != parent_task_id_) {
    // This task does not belong to this parent task ID - do nothing.
    return Glib::RefPtr<Gtk::Widget>();
  }
  if (t.is_archived() && !should_display_archived_) {
    // This task is archived - do nothing.
    return Glib::RefPtr<Gtk::Widget>();
  }
  GtkListBoxRow* row =
      reinterpret_cast<GtkListBoxRow*>(hdy_action_row_new());
  g_object_ref_sink(row);
  Glib::RefPtr<Gtk::ListBoxRow> wrapped_row(Glib::wrap(row));
  const std::string& title = t.name();
  hdy_preferences_row_set_title(
      reinterpret_cast<HdyPreferencesRow*>(wrapped_row->gobj()),
      title.c_str());
  parent_model_->CustomizeRow(wrapped_row, t);
  wrapped_row->show();
  return wrapped_row;
}

TaskListModelBase::TaskListModelBase(
    AppState* app_state, bool should_display_archived) noexcept
    : app_state_(app_state),
      should_display_archived_(should_display_archived) {
  all_connections_.emplace_back(app_state->ConnectExistingTaskChanged(
      sigc::mem_fun(*this, &TaskListModelBase::ExistingTaskChanged)));
  all_connections_.emplace_back(app_state->ConnectAfterTaskAdded(
      sigc::mem_fun(*this, &TaskListModelBase::AfterTaskAdded)));
  all_connections_.emplace_back(app_state->ConnectBeforeTaskDeleted(
      sigc::mem_fun(*this, &TaskListModelBase::BeforeTaskDeleted)));
}

const Glib::Quark TaskListModelBase::object_id_quark_(
    "task-list-object-id");

TaskListModelBase::~TaskListModelBase() {
  for (auto& connection : all_connections_) {
    connection.disconnect();
  }
}

void TaskListModelBase::InitContent() noexcept {
  const outcome::std_result<std::vector<Task>> maybe_tasks =
      should_display_archived_ ?
         Task::LoadAll(&app_state_->db_for_read_only()) :
         Task::LoadNotArchived(&app_state_->db_for_read_only());
  VERIFY(maybe_tasks);
  SetContent(maybe_tasks.value());
}

void TaskListModelBase::SetContent(const std::vector<Task>& tasks) noexcept {
  std::vector<Glib::RefPtr<Gtk::Widget>> row_controls;
  remove_all();
  top_level_rows_ = ExtractTopLevelRowInfos(tasks);
  for (auto& [task_id, info] : top_level_rows_) {
    CreateTopLevelRowControls(info.get());
    row_controls.emplace_back(info->task_row);
  }
  splice(0, 0, row_controls);
}

void TaskListModelBase::CreateTopLevelRowControls(
    TopLevelRowInfo* info) noexcept {
  if (info->child_tasks.empty()) {
    CreateNoChildToplevelRow(info);
  } else {
    CreateParentRowControls(info);
  }
  SetRowId(info->task_row, info->task);
}

void TaskListModelBase::CreateParentRowControls(
    TopLevelRowInfo* info) noexcept {
  info->child_list_box = Glib::RefPtr<Gtk::ListBox>(new Gtk::ListBox());

  Glib::RefPtr<ChildTaskListModel> child_model =
      ChildTaskListModel::create<ChildTaskListModel>(
          app_state_, *info->task.id(), should_display_archived_, this);
  VERIFY(!info->child_tasks.empty());
  info->child_list_box->bind_model(
      child_model,
      child_model->slot_create_widget());
  info->child_list_box->show();
  child_model->SetContent(info->child_tasks);

  GtkListBoxRow* row =
      reinterpret_cast<GtkListBoxRow*>(hdy_expander_row_new());
  g_object_ref_sink(row);
  Glib::RefPtr<Gtk::ListBoxRow> wrapped_row(Glib::wrap(row));
  const std::string& title = info->task.name();
  hdy_preferences_row_set_title(
      reinterpret_cast<HdyPreferencesRow*>(wrapped_row->gobj()),
      title.c_str());
  CustomizeRow(wrapped_row, info->task);
  gtk_container_add(
      reinterpret_cast<GtkContainer*>(row),
      reinterpret_cast<GtkWidget*>(info->child_list_box->gobj()));
  info->child_list_box->signal_row_selected().connect(
      sigc::mem_fun(*this, &TaskListModelBase::OnChildTaskListRowSelected));
  wrapped_row->show();
  info->task_row = wrapped_row;
}

void TaskListModelBase::CreateNoChildToplevelRow(
    TopLevelRowInfo* info) noexcept {
  VERIFY(info->child_tasks.empty());
  GtkListBoxRow* row =
      reinterpret_cast<GtkListBoxRow*>(hdy_action_row_new());
  g_object_ref_sink(row);
  Glib::RefPtr<Gtk::ListBoxRow> wrapped_row(Glib::wrap(
      reinterpret_cast<GtkListBoxRow*>(row)));
  const std::string& title = info->task.name();
  hdy_preferences_row_set_title(
      reinterpret_cast<HdyPreferencesRow*>(wrapped_row->gobj()),
      title.c_str());
  CustomizeRow(wrapped_row, info->task);
  wrapped_row->show();
  info->task_row = wrapped_row;
  info->child_list_box.reset();
}

void TaskListModelBase::SetRowId(
    Glib::RefPtr<Gtk::Widget> task_row, const Task& t) noexcept {
  auto delete_id = [](gpointer ptr) {
    delete reinterpret_cast<Task::Id*>(ptr);
  };
  // We have to copy data in pointer rather then store id as pointer,
  // since Object may allow zero id, that we can not distinguish
  // from control without data.
  VERIFY(t.id());
  task_row->set_data(object_id_quark_, new Task::Id(*t.id()), delete_id);
}

Task::Id TaskListModelBase::GetTaskIdForRow(Gtk::ListBoxRow* row) noexcept {
  const gpointer data = row->get_data(object_id_quark_);
  VERIFY(data);
  return *static_cast<const typename Task::Id*>(data);
}

// static
std::unordered_map<Task::Id,
                   std::unique_ptr<TaskListModelBase::TopLevelRowInfo>>
    TaskListModelBase::ExtractTopLevelRowInfos(
        const std::vector<Task>& tasks) noexcept {
  std::unordered_map<Task::Id, std::unique_ptr<TopLevelRowInfo>>
      parent_task_id_to_info;
  for (const Task& t : tasks) {
    VERIFY(t.id());
    if (!t.parent_task_id()) {
      auto it = parent_task_id_to_info.find(*t.id());
      if (it == parent_task_id_to_info.end()) {
        parent_task_id_to_info.emplace(
            *t.id(), std::make_unique<TopLevelRowInfo>(t));
      }
    }
  }
  for (const Task& t : tasks) {
    if (t.parent_task_id()) {
      auto it = parent_task_id_to_info.find(*t.parent_task_id());
      VERIFY(it != parent_task_id_to_info.end());
      it->second->child_tasks.emplace_back(t);
    }
  }
  return parent_task_id_to_info;
}

void TaskListModelBase::OnChildTaskListRowSelected(
    Gtk::ListBoxRow* row) noexcept {
  // |row| may be null if parent row is just being unselected.
  if (!row) {
    SetNewSelectedTaskId(std::nullopt);
  } else {
    auto* active_child_list_box = row->get_parent();
    {
      ScopedChangeValue supperss(&signals_suppressed_, true);
      UnselectAllChilListBoxesExcept(
          static_cast<Gtk::ListBox*>(active_child_list_box));
      list_box_->unselect_row();
    }
    const auto new_task_id = ChildTaskListModel::GetObjectIdForRow(row);
    VERIFY(new_task_id);
    SetNewSelectedTaskId(new_task_id);
  }
}

void TaskListModelBase::UnselectAllChilListBoxesExcept(
    Gtk::ListBox* list_box_to_exclude) noexcept {
  for (const auto& [task_id, row_info] : top_level_rows_) {
    if (row_info->child_list_box &&
        row_info->child_list_box.get() != list_box_to_exclude) {
      row_info->child_list_box->unselect_row();
    }
  }
}

void TaskListModelBase::OnMainTaskListRowSelected(
    Gtk::ListBoxRow* row) noexcept {
  if (!row) {
    // Main row unselected.
    SetNewSelectedTaskId(std::nullopt);
    return;
  }
  const Task::Id new_task_id = GetTaskIdForRow(row);
  auto it_info = top_level_rows_.find(new_task_id);
  VERIFY(it_info != top_level_rows_.end());
  if (!it_info->second->child_tasks.empty()) {
    // One of the expander rows activited - do nothing since this handler may
    // be invoked when some of child listboxes item is clicked.
    return;
  }
  {
    ScopedChangeValue supperss(&signals_suppressed_, true);
    for (const auto& [id, info] : top_level_rows_) {
      if (info->child_list_box) {
        info->child_list_box->unselect_row();
      }
    }
  }
  SetNewSelectedTaskId(new_task_id);
}

void TaskListModelBase::BindTo(Gtk::ListBox* list_box) noexcept {
  VERIFY(!list_box_);
  VERIFY(list_box);
  list_box_ = std::move(list_box);
  list_box_->bind_model(
      Glib::RefPtr<Gio::ListStore<Gtk::Widget>>(this),
      sigc::mem_fun(*this, &TaskListModelBase::CreateWidget));

  list_box_->signal_row_selected().connect(
      sigc::mem_fun(
          *this, &TaskListModelBase::OnMainTaskListRowSelected));
}

TaskListModelBase::TopLevelRowInfo*
    TaskListModelBase::FindTopLevelRowInfoForChild(
        Task::Id child_task_id) const noexcept {
  for (const auto& [parent_task_id, parent_row_info] : top_level_rows_) {
    const auto it = std::find_if(
        parent_row_info->child_tasks.begin(),
        parent_row_info->child_tasks.end(),
        [child_task_id](const Task& t) {
          return t.id() == child_task_id;
        });
    if (it != parent_row_info->child_tasks.end()) {
      return parent_row_info.get();
    }
  }
  return nullptr;
}

void TaskListModelBase::ExistingTaskChanged(const Task& t) noexcept {
  VERIFY(t.id());
  const auto it_this_task = top_level_rows_.find(*t.id());
  const bool should_remove = (t.is_archived() && !should_display_archived_);

  TopLevelRowInfo* old_parent_row_info =
      FindTopLevelRowInfoForChild(*t.id());
  if (old_parent_row_info &&
      (old_parent_row_info->task.id() != t.parent_task_id() ||
          should_remove)) {
    HandleTaskRemovedFromParent(old_parent_row_info, t);
  }

  if (t.parent_task_id()) {
    if (!old_parent_row_info ||
        old_parent_row_info->task.id() != t.parent_task_id()) {
      // Task changed parent - ensure old and new parents have proper style
      // that is, has expander where neccessary.
      if (!should_remove) {
        const auto it_new_parent = top_level_rows_.find(*t.parent_task_id());
        VERIFY(it_new_parent != top_level_rows_.end());
        HandleTaskAddedToParent(it_new_parent->second.get(), t);
      }
    }
    if (it_this_task == top_level_rows_.end()) {
      // Child task remained child.
      return;
    }
    // Top-level task become child.
    // Only one level in hiearchy supported.
    VERIFY(it_this_task->second->child_tasks.empty());
    const guint item_pos = FindItem(*it_this_task->second);
    remove(item_pos);
    top_level_rows_.erase(it_this_task);
    return;
  }

  if (it_this_task != top_level_rows_.end()) {
    if (should_remove) {
      VERIFY(it_this_task->second->child_tasks.empty());
      const guint item_pos = FindItem(*it_this_task->second);
      remove(item_pos);
      top_level_rows_.erase(it_this_task);
    } else {
      // Existing top-level row changed.
      hdy_preferences_row_set_title(
          reinterpret_cast<HdyPreferencesRow*>(
              it_this_task->second->task_row->gobj()),
          t.name().c_str());
      ReCustomizeRow(it_this_task->second->task_row, t);
    }
  } else {
    if (!should_remove) {
      // Task were not top-level, but become such.
      AfterTaskAdded(t);
    }
  }
}

void TaskListModelBase::HandleTaskRemovedFromParent(
    TopLevelRowInfo* old_parent_row_info,
    const Task& t) noexcept {
  const auto it = std::find_if(
      old_parent_row_info->child_tasks.begin(),
      old_parent_row_info->child_tasks.end(),
      [task_id = *t.id()](const Task& child) {
        return child.id() == task_id;
      });
  VERIFY(it != old_parent_row_info->child_tasks.end());
  old_parent_row_info->child_tasks.erase(it);
  EnsureProperTopLevelControlStyle(old_parent_row_info);
}

void TaskListModelBase::HandleTaskAddedToParent(
    TopLevelRowInfo* new_parent_row_info,
    const Task& t) noexcept {
  const auto it = std::find_if(
      new_parent_row_info->child_tasks.begin(),
      new_parent_row_info->child_tasks.end(),
      [task_id = *t.id()](const Task& child) {
        return child.id() == task_id;
      });
  // No duplication allowed.
  VERIFY(it == new_parent_row_info->child_tasks.end());
  new_parent_row_info->child_tasks.emplace_back(t);
  EnsureProperTopLevelControlStyle(new_parent_row_info);
}

void TaskListModelBase::EnsureProperTopLevelControlStyle(
     TopLevelRowInfo* row_info) noexcept {
  bool need_re_create_row = false;
  if (row_info->child_tasks.empty()) {
    // Row may had children previously but lost now.
    need_re_create_row = (row_info->child_list_box.get() != nullptr);
  } else {
    // Now we have children, but no list box!.
    need_re_create_row = (row_info->child_list_box.get() == nullptr);
  }
  if (need_re_create_row) {
    const guint prev_item_pos = FindItem(*row_info);
    CreateTopLevelRowControls(row_info);
    {
      ScopedChangeValue supperss(&signals_suppressed_, true);
      splice(
          prev_item_pos,
          /* n_removals */ 1,
          /* additions */ std::vector<Glib::RefPtr<Gtk::Widget>>{
              row_info->task_row});
    }
  }
}

void TaskListModelBase::AfterTaskAdded(const Task& t) noexcept {
  VERIFY(t.id());
  if (t.is_archived() && !should_display_archived_) {
    return;
  }
  if (t.parent_task_id()) {
    const auto it = top_level_rows_.find(*t.parent_task_id());
    VERIFY(it != top_level_rows_.end());
    HandleTaskAddedToParent(it->second.get(), t);
    // All other will be handled by ChildTaskListModel.
    return;
  }
  const auto [it, added_ok] = top_level_rows_.emplace(
      *t.id(), std::make_unique<TopLevelRowInfo>(t));
  VERIFY(added_ok);
  CreateTopLevelRowControls(it->second.get());
  append(it->second->task_row);
}

void TaskListModelBase::BeforeTaskDeleted(const Task& t) noexcept {
  VERIFY(t.id());
  if (t.is_archived() && !should_display_archived_) {
    // Task should be absent in this control.
    return;
  }
  if (t.parent_task_id()) {
    const auto it = top_level_rows_.find(*t.parent_task_id());
    VERIFY(it != top_level_rows_.end());
    HandleTaskRemovedFromParent(it->second.get(), t);
    // All other will be handled by ChildTaskListModel.
    return;
  }
  const auto it = top_level_rows_.find(*t.id());
  VERIFY(it != top_level_rows_.end());
  const guint item_pos = FindItem(*it->second);
  remove(item_pos);
  top_level_rows_.erase(it);
}

guint TaskListModelBase::FindItem(
    const TopLevelRowInfo& row_info) const noexcept {
  const auto n_items = get_n_items();
  for (guint i = 0; i < n_items; ++i) {
    if (get_item(i) == row_info.task_row) {
      return i;
    }
  }
  VERIFY(false);
}

void TaskListModelBase::SetNewSelectedTaskId(
    const std::optional<Task::Id>& new_id) noexcept {
  if (!signals_suppressed_) {
    selected_task_id_ = new_id;
    sig_selected_task_id_changed_(new_id);
  }
}

void TaskListModelBase::SelectTask(
    const std::optional<Task::Id>& new_selected_task_id) noexcept {
  if (!new_selected_task_id) {
    UnselectAllChilListBoxesExcept(nullptr);
    list_box_->unselect_row();
    selected_task_id_ = new_selected_task_id;
    return;
  }
  auto it = top_level_rows_.find(*new_selected_task_id);
  if (it != top_level_rows_.end()) {
    list_box_->select_row(*it->second->task_row.get());
  } else {
    const TaskListModelBase::TopLevelRowInfo* parent_row_info =
        TaskListModelBase::FindTopLevelRowInfoForChild(*new_selected_task_id);
    VERIFY(parent_row_info);
    const auto it_child = std::find_if(
        parent_row_info->child_tasks.begin(),
        parent_row_info->child_tasks.end(),
        [new_selected_task_id](const Task& t) {
          return t.id() == new_selected_task_id;
        });
    VERIFY(it_child != parent_row_info->child_tasks.end());
    UnselectAllChilListBoxesExcept(
        parent_row_info->child_list_box.get());
    Gtk::ListBoxRow* child_list_box_row =
        parent_row_info->child_list_box->get_row_at_index(
            static_cast<int>(it_child - parent_row_info->child_tasks.begin()));
    VERIFY(child_list_box_row);
    parent_row_info->child_list_box->select_row(
        *child_list_box_row);
  }
  selected_task_id_ = new_selected_task_id;
}

}  // namespace m_time_tracker
