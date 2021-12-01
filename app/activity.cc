// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/activity.h"

#include "app/database.h"

namespace m_time_tracker {

namespace {
constexpr std::string_view kBaseSelectQuery =
    "SELECT id, task_id, start_time, end_time FROM Activities";
}  // namespace

// static
outcome::std_result<void> Activity::EnsureTableCreated(Database* db) noexcept {
  const outcome::std_result<int64_t> res = db->Execute(
      "CREATE TABLE IF NOT EXISTS Activities( "
      "  id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "  task_id INTEGER, "
      "  start_time INTEGER NOT NULL, "
      "  end_time INTEGER) ",
      std::unordered_map<std::string, Database::Param>{});
  if (!res) {
    return res.error();
  }
  return outcome::success();
}

// static
outcome::std_result<std::vector<Activity>> Activity::LoadAll(
    Database* db) noexcept {
  return LoadWithQuery(db, kBaseSelectQuery);
}

// static
outcome::std_result<std::vector<Activity>> Activity::LoadAfter(
    Database* db, const TimePoint& earliest_start_time) noexcept {
  const std::string query = std::string(kBaseSelectQuery) +
      " wheRE start_time >= " +
          std::to_string(IntFromTimePoint(earliest_start_time));
  return LoadWithQuery(db, query);
}

outcome::std_result<void> Activity::Save(Database* db) noexcept {
  if (id_) {
    const std::unordered_map<std::string, Database::Param> params = {
      {":task_id", Database::Param(task_id_)},
      {":start_time", Database::Param(IntFromTimePoint(start_time_))},
      {":end_time", Database::Param(IntFromTimePoint(end_time_))},
      {":id", Database::Param(id_)},
    };
    auto exec_result = db->Execute(
        "UPDATE Activities SET "
        " task_id=:task_id,"
        " start_time=:start_time,"
        " end_time=:end_time"
        " WHERE id=:id",
        params);
    if (!exec_result) {
      return exec_result.error();
    }
  } else {
    const std::unordered_map<std::string, Database::Param> params = {
      {":task_id", Database::Param(task_id_)},
      {":start_time", Database::Param(IntFromTimePoint(start_time_))},
      {":end_time", Database::Param(IntFromTimePoint(end_time_))},
    };
    auto exec_result = db->Execute(
        "INSERT INTO Activities(task_id, start_time, end_time) VALUES("
        " :task_id,"
        " :start_time,"
        " :end_time"
        ")",
        params);
    if (!exec_result) {
      return exec_result.error();
    }
    id_ = exec_result.value();
  }
  return outcome::success();
}

// static
outcome::std_result<std::vector<Activity>> Activity::LoadWithQuery(
    Database* db, std::string_view query) noexcept {
  auto maybe_rows = db->Select(query);
  if (!maybe_rows) {
    return maybe_rows.error();
  }
  SelectRows& rows = maybe_rows.value();
  std::vector<Activity> result;
  while (true) {
    const auto next_outcome = rows.NextRow();
    if (next_outcome == SelectRows::kOutcomeDone) {
      break;
    }
    if (!next_outcome) {
      return next_outcome.error();
    }
    result.emplace_back(CreateFromSelectRow(&rows));
  }
  return result;
}

// static
// Assumes same column order as specified by kBaseSelectQuery.
Activity Activity::CreateFromSelectRow(SelectRows* row) noexcept {
  const std::optional<int64_t> id = row->Int64Column(0);
  const std::optional<int64_t> task_id = row->Int64Column(1);
  const std::optional<int64_t> start_time = row->Int64Column(2);
  const std::optional<int64_t> end_time = row->Int64Column(3);
  // Check non-null fields.
  VERIFY(id);
  VERIFY(task_id);
  VERIFY(start_time);
  std::optional<TimePoint> end_time_point = std::nullopt;
  if (end_time) {
    end_time_point = TimePointFromInt(*end_time);
  }
  return Activity(
      *id,
      *task_id,
      TimePointFromInt(*start_time),
      end_time_point);
}

// static
outcome::std_result<Activity> Activity::LoadById(
    Database* db, Id id) noexcept {
  const std::string query = std::string(kBaseSelectQuery) +
      " WHERE id=" + std::to_string(id);
  auto maybe_activities = LoadWithQuery(db, query);
  if (!maybe_activities) {
    return maybe_activities.error();
  }
  const std::vector<Activity>& activities = maybe_activities.value();
  VERIFY(activities.size() <= 1);
  if (activities.empty()) {
    return ErrorCodes::kEmptyResults;
  }
  return activities[0];
}

// static
outcome::std_result<void> Activity::Delete(Database* db, Id id) noexcept {
  const std::unordered_map<std::string, Database::Param> params = {
    {":id", Database::Param(id)},
  };
  auto exec_result = db->Execute(
      "DELETE FROM Activities WHERE id = :id",
      params);
  if (exec_result) {
    return outcome::success();
  } else {
    return exec_result.error();
  }
}

}  // namespace m_time_tracker
