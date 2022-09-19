// Copyright 2022 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <unordered_map>

#include "app/activity.h"
#include "app/error_codes.h"

namespace m_time_tracker {

class Database;

class CSVExporter {
 public:
  CSVExporter(
     Database* db_for_read_only,
     const Activity::TimePoint& from_time,
     const Activity::TimePoint& to_time,
     const std::string& export_file_path) noexcept;

  outcome::std_result<void> Run() noexcept;

 private:
  Database* const db_for_read_only_;
  const Activity::TimePoint from_time_;
  const Activity::TimePoint to_time_;
  const std::string export_file_path_;

  outcome::std_result<void> WriteHeader(std::ofstream& out_stream) noexcept;
  outcome::std_result<void> WriteDataRow(
      std::ofstream& out_stream, const Activity& a) noexcept;
  outcome::std_result<std::string> GetEscapedTaskName(
      Task::Id task_id) noexcept;

  std::unordered_map<Task::Id, std::string> cached_escaped_task_names_;
};

}  // namespace m_time_tracker
