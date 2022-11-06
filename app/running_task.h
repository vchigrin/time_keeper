// Copyright 2022 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <optional>
#include <utility>

#include "app/activity.h"
#include "app/database.h"
#include "app/error_codes.h"
#include "app/task.h"

namespace m_time_tracker {

// Information about currently running task.
// Underlying DB table has at most 1 row.
class RunningTask {
 public:
  Task::Id task_id() const noexcept {
    return task_id_;
  }

  void set_task_id(Task::Id task_id) noexcept {
    task_id_ = task_id;
  }

  Activity::TimePoint start_time() const noexcept {
    return start_time_;
  }

  void set_start_time(Activity::TimePoint start_time) noexcept {
    start_time_ = std::move(start_time);
  }

  RunningTask(Task::Id task_id, Activity::TimePoint start_time) noexcept
      : task_id_(task_id),
        start_time_(start_time) {}

  static outcome::std_result<void> EnsureTableCreated(Database* db) noexcept;
  static outcome::std_result<std::optional<RunningTask>> Load(
      Database* db) noexcept;
  static outcome::std_result<void> Delete(Database* db) noexcept;

  outcome::std_result<void> Save(Database* db) noexcept;

 private:
  Task::Id task_id_;
  Activity::TimePoint start_time_;
};

}  // namespace m_time_tracker
