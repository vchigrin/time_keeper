// Copyright 2022 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/csv_exporter.h"

#include <cerrno>
#include <fstream>
#include <utility>
#include <vector>

#include <boost/algorithm/string/replace.hpp>
#include "app/utils.h"

namespace m_time_tracker {

namespace {

std::string EscapeString(const std::string& src) noexcept {
  // RFC 4180:
  // If double-quotes are used to enclose fields, then a double-quote
  // appearing inside a field must be escaped by preceding it with
  // another double quote.
  return "\"" + boost::replace_all_copy(src, "\"", "\"\"") + "\"";
}

std::string FormatTime(const Activity::TimePoint& time_point) noexcept {
  const std::tm local_time = TimePointToLocal(time_point);
  std::stringstream sstrm;
  sstrm << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
  return sstrm.str();
}

}  // namespace

CSVExporter::CSVExporter(
     Database* db_for_read_only,
     const Activity::TimePoint& from_time,
     const Activity::TimePoint& to_time,
     const std::string& export_file_path) noexcept
  : db_for_read_only_(db_for_read_only),
    from_time_(from_time),
    to_time_(to_time),
    export_file_path_(export_file_path) {
}

outcome::std_result<void> CSVExporter::Run() noexcept {
  outcome::std_result<std::vector<Activity>> maybe_activities =
      Activity::LoadFiltered(
          db_for_read_only_,
          std::nullopt,
          from_time_,
          to_time_);
  if (!maybe_activities) {
    return maybe_activities.error();
  }
  std::vector<Activity> activities = std::move(maybe_activities.value());
  std::sort(
      activities.begin(),
      activities.end(),
      [](const Activity& first, const Activity& second) noexcept {
        return first.start_time() < second.start_time();
      });

  std::ofstream out_stream;
  out_stream.open(export_file_path_.c_str());
  if (!out_stream.good()) {
    return std::error_code(errno, std::system_category());
  }

  auto maybe_result = WriteHeader(out_stream);
  if (!maybe_result) {
    return maybe_result.error();
  }
  for (const Activity& activity : activities) {
    maybe_result = WriteDataRow(out_stream, activity);
    if (!maybe_result) {
      return maybe_result.error();
    }
  }
  out_stream.close();
  if (!out_stream.good()) {
    return std::error_code(errno, std::system_category());
  }
  return outcome::success();
}

outcome::std_result<void> CSVExporter::WriteHeader(
    std::ofstream& out_stream) noexcept {
  out_stream << "Start time,End time,Task name,Parent task name\r\n";
  if (!out_stream.good()) {
    return std::error_code(errno, std::system_category());
  }
  return outcome::success();
}

outcome::std_result<void> CSVExporter::WriteDataRow(
    std::ofstream& out_stream, const Activity& a) noexcept {
  const outcome::std_result<TaskNames> maybe_task_name =
      GetEscapedTaskName(a.task_id());
  if (!maybe_task_name) {
    return maybe_task_name.error();
  }
  const std::string start_time = FormatTime(a.start_time());
  VERIFY(a.end_time());
  const std::string end_time = FormatTime(*a.end_time());
  const auto& task_name = maybe_task_name.value();
  out_stream
      << start_time << ","
      << end_time << ","
      << task_name.task_name << ","
      << task_name.parent_task_name.value_or("")
      << "\r\n";
  if (!out_stream.good()) {
    return std::error_code(errno, std::system_category());
  }
  return outcome::success();
}

outcome::std_result<CSVExporter::TaskNames> CSVExporter::GetEscapedTaskName(
    Task::Id task_id) noexcept {
  const auto it_cached = cached_escaped_task_names_.find(task_id);
  if (it_cached != cached_escaped_task_names_.end()) {
    return it_cached->second;
  }
  const auto maybe_task = Task::LoadById(db_for_read_only_, task_id);
  if (!maybe_task) {
    return maybe_task.error();
  }
  TaskNames result;
  result.task_name = EscapeString(maybe_task.value().name());
  if (maybe_task.value().parent_task_id()) {
    const auto maybe_parent = Task::LoadById(
        db_for_read_only_, *maybe_task.value().parent_task_id());
    if (!maybe_parent) {
      return maybe_parent.error();
    }
    result.parent_task_name = EscapeString(maybe_parent.value().name());
  }
  VERIFY(cached_escaped_task_names_.emplace(task_id, result).second);
  return result;
}

}  // namespace m_time_tracker
