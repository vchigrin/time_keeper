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
  outcome::std_result<void> SaveActivity(Activity* activity) noexcept;

  using SignalWithTask = sigc::signal<void(const Task&)>;
  using SlotWithTask = SignalWithTask::slot_type;

  sigc::connection ConnectExistingTaskChanged(SlotWithTask&& h) noexcept {
    return sig_existing_task_changed_.connect(std::forward<SlotWithTask>(h));
  }

  sigc::connection ConnectBeforeTaskDeleted(SlotWithTask&& h) noexcept {
    return sig_before_task_deleted_.connect(std::forward<SlotWithTask>(h));
  }

  sigc::connection ConnectAfterTaskAdded(SlotWithTask&& h) noexcept {
    return sig_after_task_added_.connect(std::forward<SlotWithTask>(h));
  }

  Database& db_for_read_only() noexcept {
    return db_;
  }

  DbWrapper(DbWrapper&&) = default;

 private:
  // Expects DB with all tables alaready created.
  explicit DbWrapper(Database initalized_db) noexcept
      : db_(std::move(initalized_db)) {}

  SignalWithTask sig_existing_task_changed_;
  SignalWithTask sig_before_task_deleted_;
  SignalWithTask sig_after_task_added_;
  Database db_;
};

}  // namespace m_time_tracker
