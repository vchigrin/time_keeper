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

class AppState {
 public:
  static outcome::std_result<AppState> Open(
      const std::filesystem::path& db_path) noexcept;

  outcome::std_result<void> SaveTask(Task* task) noexcept;
  // Activity must be already present in DB - new activities must be added
  // through |RecordRunningTaskActivity|.
  outcome::std_result<void> SaveChangedActivity(Activity* activity) noexcept;

  using SignalWithTask = sigc::signal<void(const Task&)>;
  using SlotWithTask = SignalWithTask::slot_type;
  using SignalWithOptTask = sigc::signal<void(const std::optional<Task>&)>;
  using SlotWithOptTask = SignalWithOptTask::slot_type;
  using SignalWithActivity = sigc::signal<void(const Activity&)>;
  using SlotWithActivity = SignalWithActivity::slot_type;

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

  sigc::connection ConnectExistingActivityChanged(
      SlotWithActivity&& h) noexcept {
    return sig_existing_activity_changed_.connect(
        std::forward<SlotWithActivity>(h));
  }

  sigc::connection ConnectBeforeActivityDeleted(SlotWithActivity&& h) noexcept {
    return sig_before_activity_deleted_.connect(
        std::forward<SlotWithActivity>(h));
  }

  sigc::connection ConnectAfterActivityAdded(SlotWithActivity&& h) noexcept {
    return sig_after_activity_added_.connect(
        std::forward<SlotWithActivity>(h));
  }


  Database& db_for_read_only() noexcept {
    return db_;
  }

  AppState(AppState&&) = default;

  // Returns nullopt if there is no running Task.
  std::optional<Task> running_task() const noexcept {
    return running_task_;
  }

  // Must always be valid Task id. Previous task is silently dropped,
  // if any.
  outcome::std_result<void> StartRunningTask(Task new_task_id) noexcept;
  outcome::std_result<void> DropRunningTask() noexcept;

  // Makes Activity record about current task and continues running
  // the same task.
  outcome::std_result<void> RecordRunningTaskActivity() noexcept;

  // Changes Task without resetting run time.
  // Does not make complete Activity record in the DB -
  // use |RecordRunningTaskActivity| for that.
  outcome::std_result<void> ChangeRunningTask(Task new_task) noexcept;

  // Returns nullopt if there is no running Task.
  std::optional<Activity::Duration> RunningTaskRunTime() const noexcept;
  outcome::std_result<void> DeleteActivity(const Activity& activity) noexcept;

 private:
  // Expects DB with all tables alaready created.
  AppState(Database initalized_db,
           std::optional<Task> running_task,
           std::optional<Activity::TimePoint> running_task_start_time) noexcept
      : db_(std::move(initalized_db)),
        running_task_(std::move(running_task)),
        running_task_start_time_(std::move(running_task_start_time)) {
    if (running_task_) {
      VERIFY(running_task_start_time_);
    } else {
      VERIFY(!running_task_start_time_);
    }
  }

  SignalWithTask sig_existing_task_changed_;
  SignalWithTask sig_before_task_deleted_;
  SignalWithTask sig_after_task_added_;
  SignalWithOptTask sig_running_task_changed_;
  SignalWithActivity sig_existing_activity_changed_;
  SignalWithActivity sig_before_activity_deleted_;
  SignalWithActivity sig_after_activity_added_;
  Database db_;

  // Must alwasy be saved task.
  std::optional<Task> running_task_;
  std::optional<Activity::TimePoint> running_task_start_time_;
};

}  // namespace m_time_tracker
