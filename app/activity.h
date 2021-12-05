// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "app/error_codes.h"
#include "app/select_rows.h"
#include "app/task.h"
#include "app/verify.h"

namespace m_time_tracker {

class Database;

class Activity {
 public:
  using Id = int64_t;
  using Duration = std::chrono::seconds;
  // In UTC.
  using TimePoint =
      std::chrono::time_point<std::chrono::system_clock, Duration>;
  struct StatEntry {
    StatEntry(Task::Id id, Duration d) noexcept
        : task_id(id),
          duration(d) {}

    Task::Id task_id;
    Duration duration;
  };

  static outcome::std_result<void> EnsureTableCreated(Database* db) noexcept;

  // Useful for tests.
  static outcome::std_result<std::vector<Activity>>
      LoadAll(Database* db) noexcept;

  // Loads all tasks with start time >= earliest_start_time.
  static outcome::std_result<std::vector<Activity>> LoadAfter(
      Database* db, const TimePoint& earliest_start_time) noexcept;
  static outcome::std_result<Activity> LoadById(Database* db, Id id) noexcept;

  // Returns statistics for specified interval.
  // If some Activity overlaps interval boundary, only part of the
  // activity that in the boundary will be accounted.
  // Not completed activities are not accounted.
  static outcome::std_result<std::vector<StatEntry>>
      LoadStatsForInterval(
          Database* db,
          const TimePoint& interval_start,
          const TimePoint& interval_end) noexcept;
  static outcome::std_result<void> Delete(Database* db, Id id) noexcept;

  outcome::std_result<void> Save(Database* db) noexcept;

  Activity(const Task& task, const TimePoint& start_time) noexcept
     : start_time_(start_time) {
    VERIFY(task.id());
    task_id_ = *task.id();
  }

  void SetTask(const Task& t) noexcept {
    VERIFY(t.id());  // Task must be saved.
    task_id_ = *t.id();
  }

  void SetTaskId(const Task::Id task_id) noexcept {
    task_id_ = task_id;
  }

  std::optional<Id> id() const noexcept {
    return id_;
  }

  Task::Id task_id() const noexcept {
    return task_id_;
  }

  const TimePoint& start_time() const noexcept {
    return start_time_;
  }

  const std::optional<TimePoint> end_time() const noexcept {
    return end_time_;
  }

  void SetInterval(const TimePoint& start, const TimePoint& end) noexcept {
    start_time_ = start;
    end_time_ = end;
  }

  static TimePoint GetCurrentTimePoint() noexcept {
    return std::chrono::floor<Duration>(std::chrono::system_clock::now());
  }

 private:
  Activity(Id id,
           Task::Id task_id,
           const TimePoint& start_time,
           const std::optional<TimePoint>& end_time) noexcept
      : id_(id),
        task_id_(task_id),
        start_time_(start_time),
        end_time_(end_time) {}

  static outcome::std_result<std::vector<Activity>> LoadWithQuery(
      Database* db, std::string_view query) noexcept;
  static Activity CreateFromSelectRow(SelectRows* row) noexcept;

  static TimePoint TimePointFromInt(int64_t val) noexcept {
    const std::time_t time_val = static_cast<std::time_t>(val);
    static_assert(
        sizeof(time_val) == sizeof(int64_t),
        "Assume time_t is number of seconds since Jan 1. 1970");
    const std::chrono::system_clock::time_point system_time_point =
        std::chrono::system_clock::from_time_t(time_val);
    return std::chrono::floor<std::chrono::seconds>(system_time_point);
  }

  static int64_t IntFromTimePoint(const TimePoint& tp) noexcept {
    const std::time_t time_val = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::time_point(tp));
    static_assert(
        sizeof(time_val) == sizeof(int64_t),
        "Assume time_t is number of seconds since Jan 1. 1970");
    return static_cast<int64_t>(time_val);
  }

  static Duration DurationFromInt(int64_t duration) noexcept {
    return std::chrono::seconds(duration);
  }

  static std::optional<int64_t> IntFromTimePoint(
      const std::optional<TimePoint>& tp) noexcept {
    if (!tp) {
      return std::nullopt;
    } else {
      return IntFromTimePoint(*tp);
    }
  }

  std::optional<Id> id_;
  Task::Id task_id_;
  TimePoint start_time_;
  std::optional<TimePoint> end_time_;
};

}  // namespace m_time_tracker
