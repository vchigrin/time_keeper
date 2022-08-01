// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/app_state.h"

#include "app/running_task.h"

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
  const auto running_task_init_result =
        RunningTask::EnsureTableCreated(&maybe_db.value());
  if (!running_task_init_result) {
    return running_task_init_result.error();
  }
  const auto maybe_running_result = RunningTask::Load(&maybe_db.value());
  if (!maybe_running_result) {
    return maybe_running_result.error();
  }
  const std::optional<RunningTask>& maybe_running =
      maybe_running_result.value();
  if (!maybe_running) {
    return AppState(std::move(maybe_db.value()), std::nullopt, std::nullopt);
  } else {
    const auto maybe_task = Task::LoadById(
        &maybe_db.value(),
        maybe_running->task_id());
    if (!maybe_task) {
      return maybe_task.error();
    }
    return AppState(
        std::move(maybe_db.value()),
        maybe_task.value(),
        maybe_running->start_time());
  }
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

outcome::std_result<void> AppState::SaveChangedActivity(
    Activity* activity) noexcept {
  VERIFY(activity);
  VERIFY(activity->id());
  outcome::std_result<void> save_result = activity->Save(&db_);
  if (save_result) {
    sig_existing_activity_changed_(*activity);
  }
  return save_result;
}

outcome::std_result<void> AppState::StartRunningTask(Task new_task) noexcept {
  VERIFY(new_task.id());
  running_task_ = std::move(new_task);
  running_task_start_time_ = Activity::GetCurrentTimePoint();
  sig_running_task_changed_(running_task_);
  RunningTask rt(*running_task_->id(), *running_task_start_time_);
  return rt.Save(&db_);
}

outcome::std_result<void> AppState::DropRunningTask() noexcept {
  const auto db_result = RunningTask::Delete(&db_);
  if (!db_result) {
    return db_result;
  }
  running_task_ = std::nullopt;
  running_task_start_time_ = std::nullopt;
  sig_running_task_changed_(running_task_);
  return outcome::success();
}

outcome::std_result<void> AppState::RecordRunningTaskActivity() noexcept {
  VERIFY(running_task_);
  VERIFY(running_task_start_time_);
  Activity new_activity(*running_task_, *running_task_start_time_);
  const Activity::TimePoint now = Activity::GetCurrentTimePoint();
  new_activity.SetInterval(*running_task_start_time_, now);
  const auto db_result = new_activity.Save(&db_);
  if (!db_result) {
    return db_result;
  }
  running_task_start_time_ = now;
  sig_after_activity_added_(new_activity);
  return outcome::success();
}

outcome::std_result<void> AppState::ChangeRunningTask(Task new_task) noexcept {
  if (!running_task_) {
    const auto start_result = StartRunningTask(std::move(new_task));
    if (!start_result) {
      return start_result;
    }
  } else {
    VERIFY(new_task.id());
    running_task_ = std::move(new_task);
    RunningTask rt(*new_task.id(), *running_task_start_time_);
    const auto db_result = rt.Save(&db_);
    if (!db_result) {
      return db_result;
    }
  }
  sig_running_task_changed_(running_task_);
  return outcome::success();
}

std::optional<Activity::Duration>
    AppState::RunningTaskRunTime() const noexcept {
  if (!running_task_start_time_) {
    return std::nullopt;
  } else {
    return Activity::GetCurrentTimePoint() - *running_task_start_time_;
  }
}

outcome::std_result<void> AppState::DeleteActivity(
    const Activity& activity) noexcept {
  VERIFY(activity.id());
  sig_before_activity_deleted_(activity);
  return Activity::Delete(&db_, *activity.id());
}

}  // namespace m_time_tracker
