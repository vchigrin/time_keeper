// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/app_state.h"

namespace m_time_tracker {

// static
outcome::std_result<AppState> AppState::Open(
    const std::filesystem::path& db_path) noexcept {
  outcome::std_result<Database> maybe_db = Database::Open(db_path);
  if (!maybe_db) {
    return maybe_db.error();
  }
  const auto task_init_result = Task::EnsureTableCreated(&maybe_db.value());
  if (!task_init_result) {
    return task_init_result.error();
  }
  const auto activity_init_result =
      Activity::EnsureTableCreated(&maybe_db.value());
  if (!activity_init_result) {
    return activity_init_result.error();
  }
  return AppState(std::move(maybe_db.value()));
}

outcome::std_result<void> AppState::SaveTask(Task* task) noexcept {
  VERIFY(task);
  const bool was_saved = (task->id() != std::nullopt);
  outcome::std_result<void> save_result = task->Save(&db_);
  if (save_result) {
    if (running_task_ && running_task_->id() == task->id()) {
      running_task_ = *task;
      VERIFY(running_task_->id());
    }
    if (was_saved) {
      sig_existing_task_changed_(*task);
    } else {
      sig_after_task_added_(*task);
    }
  }
  return save_result;
}

void AppState::StartRunningTask(Task new_task) noexcept {
  VERIFY(new_task.id());
  running_task_ = std::move(new_task);
  running_task_start_time_ = Activity::GetCurrentTimePoint();
  sig_running_task_changed_(running_task_);
}

void AppState::DropRunningTask() noexcept {
  running_task_ = std::nullopt;
  running_task_start_time_ = std::nullopt;
  sig_running_task_changed_(running_task_);
}

outcome::std_result<void> AppState::RecordRunningTaskActivity() noexcept {
  VERIFY(running_task_);
  VERIFY(running_task_start_time_);
  Activity new_activity(*running_task_, *running_task_start_time_);
  const Activity::TimePoint now = Activity::GetCurrentTimePoint();
  new_activity.SetInterval(*running_task_start_time_, now);
  const auto outcome = new_activity.Save(&db_);
  if (!outcome) {
    return outcome;
  }
  running_task_start_time_ = now;
  sig_after_activity_added_(new_activity);
  return outcome;
}

void AppState::ChangeRunningTask(Task new_task) noexcept {
  if (!running_task_) {
    StartRunningTask(std::move(new_task));
  } else {
    VERIFY(new_task.id());
    running_task_ = std::move(new_task);
  }
  sig_running_task_changed_(running_task_);
}

std::optional<Activity::Duration>
    AppState::RunningTaskRunTime() const noexcept {
  if (!running_task_start_time_) {
    return std::nullopt;
  } else {
    return Activity::GetCurrentTimePoint() - *running_task_start_time_;
  }
}

}  // namespace m_time_tracker
