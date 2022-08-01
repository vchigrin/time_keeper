// Copyright 2022 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/running_task.h"

#include <string>
#include <unordered_map>

namespace m_time_tracker {

// static
outcome::std_result<void> RunningTask::EnsureTableCreated(
    Database* db) noexcept {
  const auto res = db->Execute(
      "CREATE TABLE IF NOT EXISTS RunningTask( "
      "  task_id INTEGER PRIMARY KEY, "
      "  start_time INTEGER NOT NULL) ",
      std::unordered_map<std::string, Database::Param>{});
  if (!res) {
    return res.error();
  }
  return outcome::success();
}

// static
outcome::std_result<std::optional<RunningTask>>
    RunningTask::Load(Database* db) noexcept {
  auto maybe_rows = db->Select("SELECT task_id, start_time FROM RunningTask");
  if (!maybe_rows) {
    return maybe_rows.error();
  }
  SelectRows& row = maybe_rows.value();
  const auto next_outcome = row.NextRow();
  if (next_outcome == SelectRows::kOutcomeDone) {
    return std::nullopt;
  }
  if (!next_outcome) {
    return next_outcome.error();
  }
  const std::optional<int64_t> task_id = row.Int64Column(0);
  const std::optional<int64_t> start_time = row.Int64Column(1);
  VERIFY(task_id);
  VERIFY(start_time);
  return std::optional<RunningTask>(RunningTask(
      *task_id,
      Activity::TimePointFromInt(*start_time)));
}

outcome::std_result<void> RunningTask::Delete(Database* db) noexcept {
  auto maybe_result = db->Execute(
      "DELETE FROM RunningTask",
      std::unordered_map<std::string, Database::Param>{});
  if (!maybe_result) {
    return maybe_result.error();
  }
  return outcome::success();
}

outcome::std_result<void> RunningTask::Save(Database* db) noexcept {
  const auto delete_result = Delete(db);
  if (!delete_result) {
    return delete_result.error();
  }
  const std::unordered_map<std::string, Database::Param> params = {
    {":task_id", Database::Param(task_id_)},
    {":start_time", Database::Param(Activity::IntFromTimePoint(start_time_))},
  };
  const auto maybe_result = db->Execute(
      "INSERT INTO RunningTask(task_id, start_time) "
      "VALUES(:task_id, :start_time)",
      params);
  if (!maybe_result) {
    return maybe_result.error();
  }
  return outcome::success();
}

}  // namespace m_time_tracker
