// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/task_list_model_base.h"

#include <utility>

#include "app/db_wrapper.h"

namespace m_time_tracker {

TaskListModelBase::TaskListModelBase(DbWrapper* db_wrapper) noexcept {
  all_connections_.emplace_back(db_wrapper->ConnectExistingTaskChanged(
      sigc::mem_fun(*this, &TaskListModelBase::ExistingTaskChanged)));
  all_connections_.emplace_back(db_wrapper->ConnectAfterTaskAdded(
      sigc::mem_fun(*this, &TaskListModelBase::AfterTaskAdded)));
  all_connections_.emplace_back(db_wrapper->ConnectBeforeTaskDeleted(
      sigc::mem_fun(*this, &TaskListModelBase::BeforeTaskDeleted)));
}

TaskListModelBase::~TaskListModelBase() {
  for (auto& connection : all_connections_) {
    connection.disconnect();
  }
}

void TaskListModelBase::Initialize(const std::vector<Task>& tasks) noexcept {
  VERIFY(task_id_to_item_index_.empty());
  std::vector<Glib::RefPtr<Gtk::Widget>> items;
  items.reserve(tasks.size());
  for (const Task& t : tasks) {
    VERIFY(t.id());
    auto control = CreateRowFromTask(t);
    VERIFY(control);
    items.push_back(std::move(control));
    VERIFY(task_id_to_item_index_.insert({*t.id(), items.size()}).second);
  }
  splice(0U, 0U, items);
}

void TaskListModelBase::ExistingTaskChanged(const Task& t) noexcept {
  VERIFY(t.id());
  auto it = task_id_to_item_index_.find(*t.id());
  VERIFY(it != task_id_to_item_index_.end());

  const guint item_position = it->second;

  auto new_control = CreateRowFromTask(t);
  remove(item_position);
  insert(item_position, new_control);
}

void TaskListModelBase::AfterTaskAdded(const Task& t) noexcept {
  Glib::RefPtr<Gtk::Widget> new_control = CreateRowFromTask(t);
  VERIFY(t.id());
  const guint new_item_index = get_n_items();
  const auto insert_result = task_id_to_item_index_.insert(
      {*t.id(), new_item_index});
  VERIFY(insert_result.second);
  append(new_control);
}

void TaskListModelBase::BeforeTaskDeleted(const Task& t) noexcept {
  VERIFY(t.id());
  auto it = task_id_to_item_index_.find(*t.id());
  VERIFY(it != task_id_to_item_index_.end());
  remove(it->second);
  task_id_to_item_index_.erase(it);
}

}  // namespace m_time_tracker
