// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "app/error_codes.h"
#include "app/select_rows.h"
#include "app/verify.h"

namespace m_time_tracker {

class Database;

class Task {
 public:
  static outcome::std_result<void> EnsureTableCreated(Database* db) noexcept;
  static outcome::std_result<std::vector<Task>> LoadAll(Database* db) noexcept;
  static outcome::std_result<std::vector<Task>> LoadNotArchived(
      Database* db) noexcept;
  static outcome::std_result<Task> LoadById(Database* db, int64_t id) noexcept;
  static outcome::std_result<std::vector<Task>> LoadTopLevel(
      Database* db) noexcept;
  static outcome::std_result<std::vector<Task>> LoadChildTasks(
      Database* db, const Task& parent) noexcept;

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

  std::optional<int64_t> id() const noexcept {
    return id_;
  }

  void SetParentTask(const Task& parent) noexcept {
    VERIFY(parent.id_);  // Parent task must be saved.
    parent_task_id_ = parent.id_;
  }

 private:
  Task(int64_t id,
       std::string name,
       std::optional<int64_t> parent_task_id,
       bool is_archived) noexcept
      : id_(id),
        name_(std::move(name)),
        parent_task_id_(parent_task_id),
        is_archived_(is_archived) {
  }
  static Task CreateFromSelectRow(SelectRows* row) noexcept;
  static outcome::std_result<std::vector<Task>> LoadWithQuery(
      Database* db, std::string_view query) noexcept;

  std::optional<int64_t> id_;
  std::string name_;
  std::optional<int64_t> parent_task_id_;
  bool is_archived_;
};

}  // namespace m_time_tracker
