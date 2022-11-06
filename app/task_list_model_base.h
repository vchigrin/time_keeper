// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <sigc++/sigc++.h>

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "app/task.h"
#include "app/ui_helpers.h"

namespace m_time_tracker {

class AppState;
class ChildTaskListModel;

class TaskListModelBase: public Gio::ListStore<Gtk::Widget> {
 public:
  virtual ~TaskListModelBase();

  // Control must outlive this object.
  void BindTo(Gtk::ListBox* list_box) noexcept;
  Task::Id GetTaskIdForRow(Gtk::ListBoxRow* row) noexcept;

  template<typename ChildClass, typename... Args>
  static Glib::RefPtr<ChildClass> create(Args&&... args) noexcept {
    ChildClass* result = new ChildClass(std::forward<Args>(args)...);
    // ref counter is 0 by default, so explicity add refernce here.
    result->reference();
    return Glib::RefPtr<ChildClass>(result);
  }
  using SignalWithOptTaskId =
      sigc::signal<void(const std::optional<Task::Id>&)>;
  using SlotWithOptTaskId = SignalWithOptTaskId::slot_type;

  sigc::connection ConnectSelectedTaskIdChanged(
      SlotWithOptTaskId&& h) noexcept {
    return sig_selected_task_id_changed_.connect(
        std::forward<SlotWithOptTaskId>(h));
  }

  const std::optional<Task::Id>& selected_task_id() const noexcept {
    return selected_task_id_;
  }

  void SelectTask(
      const std::optional<Task::Id>& new_selected_task_id) noexcept;

 protected:
  TaskListModelBase(AppState* app_state, bool should_display_archived) noexcept;
  friend class ChildTaskListModel;
  virtual void CustomizeRow(
      const Glib::RefPtr<Gtk::ListBoxRow>, const Task&) noexcept {}
  // Invoked when properties of Task changed. Row was already customized once
  // with |CustomizeRow|.
  virtual void ReCustomizeRow(
      const Glib::RefPtr<Gtk::ListBoxRow>, const Task&) noexcept {}

  // Must be invoked by child classes after their construction is finished.
  void InitContent() noexcept;

 private:
  struct TopLevelRowInfo {
    explicit TopLevelRowInfo(Task t) noexcept
        : task(t) {}
    TopLevelRowInfo(const TopLevelRowInfo&) = delete;
    const TopLevelRowInfo& operator=(const TopLevelRowInfo&) = delete;

    Task task;
    Glib::RefPtr<Gtk::ListBox> child_list_box;
    Glib::RefPtr<Gtk::ListBoxRow> task_row;
    // May be empty if this task does not have children.
    std::vector<Task> child_tasks;
  };

  sigc::slot<Gtk::Widget*(const Glib::RefPtr<Glib::Object>)>
      slot_create_widget() noexcept {
    return sigc::mem_fun(
        *this,
        &TaskListModelBase::CreateWidget);
  }

  Gtk::Widget* CreateWidget(const Glib::RefPtr<Glib::Object> obj) noexcept {
    // Caller responsible on releasing reference.
    obj->reference();
    return static_cast<Gtk::Widget*>(obj.get());
  }

  static std::unordered_map<Task::Id, std::unique_ptr<TopLevelRowInfo>>
      ExtractTopLevelRowInfos(const std::vector<Task>& tasks) noexcept;
  void SetContent(const std::vector<Task>& tasks) noexcept;
  void SetRowId(Glib::RefPtr<Gtk::Widget> task_row, const Task& t) noexcept;

  void ExistingTaskChanged(const Task& t) noexcept;
  void AfterTaskAdded(const Task& t) noexcept;
  void BeforeTaskDeleted(const Task& t) noexcept;

  void OnChildTaskListRowSelected(Gtk::ListBoxRow* row) noexcept;
  void OnMainTaskListRowSelected(Gtk::ListBoxRow* row) noexcept;

  void CreateParentRowControls(TopLevelRowInfo* info) noexcept;
  void CreateNoChildToplevelRow(TopLevelRowInfo* info) noexcept;
  void CreateTopLevelRowControls(TopLevelRowInfo* info) noexcept;

  void HandleTaskAddedToParent(
      TopLevelRowInfo* new_parent_row_info,
      const Task& t) noexcept;
  void HandleTaskRemovedFromParent(
      TopLevelRowInfo* new_parent_row_info,
      const Task& t) noexcept;
  void EnsureProperTopLevelControlStyle(
      TopLevelRowInfo* row_info) noexcept;
  void SetNewSelectedTaskId(
      const std::optional<Task::Id>& new_id) noexcept;

  guint FindItem(const TopLevelRowInfo& row_info) const noexcept;
  TopLevelRowInfo* FindTopLevelRowInfoForChild(
      Task::Id child_task_id) const noexcept;
  void UnselectAllChilListBoxesExcept(
      Gtk::ListBox* list_box_to_exclude) noexcept;
  guint ComputePositionForTopLevelTask(const Task& t) const noexcept;

  AppState* const app_state_;
  const bool should_display_archived_;

  static const Glib::Quark object_id_quark_;

  // Connections to AppState signals..
  std::vector<sigc::connection> all_connections_;
  std::unordered_map<Task::Id, std::unique_ptr<TopLevelRowInfo>>
      top_level_rows_;
  Gtk::ListBox* list_box_ = nullptr;
  bool signals_suppressed_ = false;
  std::optional<Task::Id> selected_task_id_;

  SignalWithOptTaskId sig_selected_task_id_changed_;
};

}  // namespace m_time_tracker
