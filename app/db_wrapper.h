// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <sigc++/sigc++.h>
#include <utility>

#include "app/database.h"
#include "app/activity.h"
#include "app/task.h"

namespace m_time_tracker {

class DbWrapper {
 public:
  static outcome::std_result<DbWrapper> Open(
      const std::filesystem::path& db_path) noexcept;

  outcome::std_result<void> SaveTask(Task* task) noexcept;

  using SignalWithTask = sigc::signal<void(const Task&)>;
  using SlotWithTask = SignalWithTask::slot_type;
  using SignalWithOptTask = sigc::signal<void(const std::optional<Task>&)>;
  using SlotWithOptTask = SignalWithOptTask::slot_type;

  sigc::connection ConnectExistingTaskChanged(SlotWithTask&& h) noexcept {
    return sig_existing_task_changed_.connect(std::forward<SlotWithTask>(h));
  }

  sigc::connection ConnectBeforeTaskDeleted(SlotWithTask&& h) noexcept {
    return sig_before_task_deleted_.connect(std::forward<SlotWithTask>(h));
  }

  sigc::connection ConnectAfterTaskAdded(SlotWithTask&& h) noexcept {
    return sig_after_task_added_.connect(std::forward<SlotWithTask>(h));
  }

  sigc::connection ConnectRunningTaskChanged(SlotWithOptTask&& h) noexcept {
    return sig_running_task_changed_.connect(std::forward<SlotWithOptTask>(h));
  }

  Database& db_for_read_only() noexcept {
    return db_;
  }

  DbWrapper(DbWrapper&&) = default;

  // Returns nullopt if there is no running Task.
  std::optional<Task> running_task() const noexcept {
    return running_task_;
  }

  // Must always be valid Task id. Previous task is silently dropped,
  // if any.
  void StartRunningTask(Task new_task_id) noexcept;
  void DropRunningTask() noexcept;

  // Makes Activity record about current task and continues running
  // the same task.
  outcome::std_result<void> RecordRunningTaskActivity() noexcept;

  // Changes Task without resetting run time.
  // Does not make complete Activity record in the DB -
  // use |record_running_task_activity| for that.
  void ChangeRunningTask(Task new_task) noexcept;

  // Returns nullopt if there is no running Task.
  std::optional<Activity::Duration> RunningTaskRunTime() const noexcept;

 private:
  // Expects DB with all tables alaready created.
  explicit DbWrapper(Database initalized_db) noexcept
      : db_(std::move(initalized_db)) {}

  SignalWithTask sig_existing_task_changed_;
  SignalWithTask sig_before_task_deleted_;
  SignalWithTask sig_after_task_added_;
  SignalWithOptTask sig_running_task_changed_;
  Database db_;

  // Must alwasy be saved task.
  std::optional<Task> running_task_;
  std::optional<Activity::TimePoint> running_task_start_time_;
};

}  // namespace m_time_tracker
