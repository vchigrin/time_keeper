// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "app/database.h"
#include "app/error_codes.h"
#include "app/select_rows.h"
#include "app/verify.h"

namespace m_time_tracker {

class Task {
 public:
  using Id = int64_t;

  static outcome::std_result<void> EnsureTableCreated(Database* db) noexcept;
  static outcome::std_result<std::vector<Task>> LoadAll(Database* db) noexcept;
  static outcome::std_result<std::vector<Task>> LoadNotArchived(
      Database* db) noexcept;
  static outcome::std_result<Task> LoadById(Database* db, Id id) noexcept;
  static outcome::std_result<Task> LoadByName(
      Database* db, const std::string& name) noexcept;
  static outcome::std_result<std::vector<Task>> LoadTopLevel(
      Database* db) noexcept;
  static outcome::std_result<std::vector<Task>> LoadChildTasks(
      Database* db, const Task& parent) noexcept;
  static outcome::std_result<int64_t> ChildTasksCount(
      Database* db,
      Task::Id task_id) noexcept;

  outcome::std_result<void> Save(Database* db) noexcept;

  // New task object, without parent task object, not archived.
  explicit Task(std::string name) noexcept
      : name_(std::move(name)),
        is_archived_(false) {
  }

  const std::string& name() const noexcept {
    return name_;
  }

  void set_name(const std::string& name) noexcept {
    VERIFY(!name.empty());
    name_ = std::move(name);
  }

  bool is_archived() const noexcept {
    return is_archived_;
  }

  void set_archived(bool v) noexcept {
    is_archived_ = v;
  }

  std::optional<Id> id() const noexcept {
    return id_;
  }

  std::optional<Id> parent_task_id() const noexcept {
    return parent_task_id_;
  }

  void set_parent_task_id(std::optional<Task::Id> id) noexcept {
    parent_task_id_ = id;
  }

  void SetParentTask(const Task& parent) noexcept {
    VERIFY(parent.id_);  // Parent task must be saved.
    set_parent_task_id(parent.id_);
  }

 private:
  Task(Id id,
       std::string name,
       std::optional<Id> parent_task_id,
       bool is_archived) noexcept
      : id_(id),
        name_(std::move(name)),
        parent_task_id_(parent_task_id),
        is_archived_(is_archived) {
  }
  static Task CreateFromSelectRow(SelectRows* row) noexcept;
  static outcome::std_result<std::vector<Task>> LoadWithQuery(
      Database* db, std::string_view query,
      const std::unordered_map<
          std::string, Database::Param>& params = {}) noexcept;

  std::optional<Id> id_;
  std::string name_;
  std::optional<Id> parent_task_id_;
  bool is_archived_;
};

}  // namespace m_time_tracker
