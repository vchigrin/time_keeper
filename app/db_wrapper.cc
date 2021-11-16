// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/db_wrapper.h"

namespace m_time_tracker {

// static
outcome::std_result<DbWrapper> DbWrapper::Open(
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
  return DbWrapper(std::move(maybe_db.value()));
}

outcome::std_result<void> DbWrapper::SaveTask(Task* task) noexcept {
  VERIFY(task);
  outcome::std_result<void> save_result = task->Save(&db_);
  if (save_result) {
    signal_task_list_changed_.emit();
  }
  return save_result;
}

outcome::std_result<void> DbWrapper::SaveActivity(Activity* activity) noexcept {
  VERIFY(activity);
  return activity->Save(&db_);
}

}  // namespace m_time_tracker
