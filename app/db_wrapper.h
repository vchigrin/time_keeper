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

  template<typename Handler>
  sigc::connection ConnectToTaskListChanged(Handler&& h) noexcept {
    return signal_task_list_changed_.connect(std::forward<Handler>(h));
  }

  Database& db_for_read_only() noexcept {
    return db_;
  }

  DbWrapper(DbWrapper&&) = default;

 private:
  // Expects DB with all tables alaready created.
  explicit DbWrapper(Database initalized_db) noexcept
      : db_(std::move(initalized_db)) {}

  sigc::signal<void()> signal_task_list_changed_;
  Database db_;
};

}  // namespace m_time_tracker
