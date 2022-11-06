// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/activity.h"

#include <boost/algorithm/string.hpp>

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
      " WHERE start_time >= " +
          std::to_string(IntFromTimePoint(earliest_start_time));
  return LoadWithQuery(db, query);
}

// static
outcome::std_result<std::vector<Activity>> Activity::LoadFiltered(
    Database* db,
    const std::optional<Task::Id> task_id,
    const std::optional<TimePoint> earliest_start_time,
    const std::optional<TimePoint> latest_start_time) noexcept {
  std::vector<std::string> conditions;
  if (task_id) {
    conditions.push_back("task_id = " + std::to_string(*task_id));
  }
  if (earliest_start_time) {
    conditions.push_back("start_time >= " +
        std::to_string(IntFromTimePoint(*earliest_start_time)));
  }
  if (latest_start_time) {
    conditions.push_back("start_time <= " +
        std::to_string(IntFromTimePoint(*latest_start_time)));
  }
  std::string query = std::string(kBaseSelectQuery) +
      " WHERE end_time IS NOT NULL ";
  if (!conditions.empty()) {
    query += " AND " + boost::algorithm::join(conditions, " AND ");
  }

  return LoadWithQuery(db, query);
}

outcome::std_result<void> Activity::Save(Database* db) noexcept {
  VERIFY(!end_time_ || *end_time_ > start_time_);
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
outcome::std_result<std::optional<Activity::TimePoint>>
    Activity::LoadEarliestActivityStart(Database* db) noexcept {
  auto maybe_rows = db->Select("SELECT min(start_time) FROM Activities");
  if (!maybe_rows) {
    return maybe_rows.error();
  }
  SelectRows& rows = maybe_rows.value();
  const auto next_outcome = rows.NextRow();
  if (!next_outcome) {
    return next_outcome.error();
  }
  const std::optional<int64_t> start_time = rows.Int64Column(0);
  if (!start_time) {
    return std::nullopt;
  }
  return TimePointFromInt(*start_time);
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

// static
outcome::std_result<std::vector<Activity::StatEntry>>
    Activity::LoadStatsForInterval(
        Database* db,
        const TimePoint& interval_start,
        const TimePoint& interval_end,
        Task::Id parent_task_id) noexcept {
  if (interval_start >= interval_end) {
    return std::vector<StatEntry>{};
  }
  static constexpr std::string_view kQuery = (
      "SELECT task_id, "
      " SUM(MIN(end_time, :interval_end) - MAX(start_time, :interval_start)) "
      " FROM Activities, Tasks "
      " WHERE "
      " Activities.task_id = Tasks.id AND "
      " Tasks.parent_task_id = :parent_task_id AND "
      "((end_time >= :interval_start AND end_time < :interval_end) OR "
      "  (start_time >= :interval_start AND start_time < :interval_end) OR "
      "  (start_time < :interval_start AND end_time > :interval_end)) "
      " GROUP BY task_id");
  const std::unordered_map<std::string, Database::Param> params = {
      {":interval_start", Database::Param(IntFromTimePoint(interval_start))},
      {":interval_end", Database::Param(IntFromTimePoint(interval_end))},
      {":parent_task_id", Database::Param(parent_task_id)},
  };
  return StatsFromSelectResults(db->Select(kQuery, params));
}

outcome::std_result<std::vector<Activity::StatEntry>>
    Activity::LoadStatsForTopLevelTasksInInterval(
        Database* db,
        const TimePoint& interval_start,
        const TimePoint& interval_end) noexcept {
  if (interval_start >= interval_end) {
    return std::vector<StatEntry>{};
  }
  static constexpr std::string_view kQuery = (
      "SELECT "
      " (CASE WHEN parent_task_id is not NULL THEN "
      "    parent_task_id ELSE Tasks.id END) AS group_id,"
      " SUM(MIN(end_time, :interval_end) - MAX(start_time, :interval_start)) "
      " FROM Activities, Tasks "
      " WHERE "
      " Activities.task_id = Tasks.id AND "
      "((end_time >= :interval_start AND end_time < :interval_end) OR "
      "  (start_time >= :interval_start AND start_time < :interval_end) OR "
      "  (start_time < :interval_start AND end_time > :interval_end)) "
      " GROUP BY group_id");
  const std::unordered_map<std::string, Database::Param> params = {
      {":interval_start", Database::Param(IntFromTimePoint(interval_start))},
      {":interval_end", Database::Param(IntFromTimePoint(interval_end))},
  };
  return StatsFromSelectResults(db->Select(kQuery, params));
}

// static
outcome::std_result<std::vector<Activity::StatEntry>>
    Activity::StatsFromSelectResults(
        outcome::std_result<SelectRows> maybe_rows) noexcept {
  if (!maybe_rows) {
    return maybe_rows.error();
  }
  SelectRows& rows = maybe_rows.value();
  std::vector<StatEntry> result;
  while (true) {
    const auto next_outcome = rows.NextRow();
    if (next_outcome == SelectRows::kOutcomeDone) {
      break;
    }
    if (!next_outcome) {
      return next_outcome.error();
    }
    const std::optional<int64_t> task_id = rows.Int64Column(0);
    const std::optional<int64_t> duration = rows.Int64Column(1);
    VERIFY(task_id);
    VERIFY(duration);
    result.emplace_back(*task_id, DurationFromInt(*duration));
  }
  return result;
}

}  // namespace m_time_tracker
