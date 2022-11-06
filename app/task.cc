// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/task.h"

#include <unordered_map>
#include <utility>

#include "app/database.h"

namespace m_time_tracker {

namespace {
constexpr std::string_view kBaseSelectQuery =
    "SELECT id, name, is_archived, parent_task_id FROM Tasks";
}  // namespace

// static
outcome::std_result<void> Task::EnsureTableCreated(Database* db) noexcept {
  const outcome::std_result<Id> res = db->Execute(
      "CREATE TABLE IF NOT EXISTS Tasks( "
      "  id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "  name TEXT UNIQUE NOT NULL, "
      "  parent_task_id INTEGER, "
      "  is_archived INTEGER) ",
      std::unordered_map<std::string, Database::Param>{});
  if (!res) {
    return res.error();
  }
  return outcome::success();
}

// static
// Assumes same column order as specified by kBaseSelectQuery.
Task Task::CreateFromSelectRow(SelectRows* row) noexcept {
  const std::optional<int64_t> id = row->Int64Column(0);
  std::optional<std::string> name = row->StringColumn(1);
  const std::optional<int> is_archived = row->IntColumn(2);
  const std::optional<int64_t> parent_task_id = row->Int64Column(3);
  // Check non-null fields.
  VERIFY(id);
  VERIFY(name);
  VERIFY(is_archived);
  return Task(
      *id,
      std::move(*name),
      parent_task_id,
      *is_archived);
}

// static
outcome::std_result<std::vector<Task>> Task::LoadAll(Database* db) noexcept {
  return LoadWithQuery(db, kBaseSelectQuery);
}

// static
outcome::std_result<std::vector<Task>> Task::LoadNotArchived(
    Database* db) noexcept {
  const std::string query = std::string(kBaseSelectQuery) +
      " WHERE is_archived=0";
  return LoadWithQuery(db, query);
}

// static
outcome::std_result<Task> Task::LoadById(Database* db, Id id) noexcept {
  const std::string query = std::string(kBaseSelectQuery) +
      " WHERE id=" + std::to_string(id);
  auto maybe_tasks = LoadWithQuery(db, query);
  if (!maybe_tasks) {
    return maybe_tasks.error();
  }
  const std::vector<Task>& tasks = maybe_tasks.value();
  VERIFY(tasks.size() <= 1);
  if (tasks.empty()) {
    return ErrorCodes::kEmptyResults;
  }
  return tasks[0];
}

// static
outcome::std_result<Task> Task::LoadByName(
    Database* db, const std::string& name) noexcept {
  const std::string query = std::string(kBaseSelectQuery) +
      " WHERE name=:name";
  const std::unordered_map<std::string, Database::Param> params = {
    {":name", Database::Param(name)},
  };
  auto maybe_tasks = LoadWithQuery(db, query, params);
  if (!maybe_tasks) {
    return maybe_tasks.error();
  }
  const std::vector<Task>& tasks = maybe_tasks.value();
  VERIFY(tasks.size() <= 1);
  if (tasks.empty()) {
    return ErrorCodes::kEmptyResults;
  }
  return tasks[0];
}

// static
outcome::std_result<std::vector<Task>> Task::LoadTopLevel(
    Database* db) noexcept {
  const std::string query = std::string(kBaseSelectQuery) +
      " WHERE parent_task_id IS NULL";
  return LoadWithQuery(db, query);
}

// static
outcome::std_result<std::vector<Task>> Task::LoadChildTasks(
      Database* db, const Task& parent) noexcept {
  VERIFY(parent.id_);  // Parent must be saved.
  const std::string query = std::string(kBaseSelectQuery) +
      " WHERE parent_task_id=" + std::to_string(*parent.id_);
  return LoadWithQuery(db, query);
}

// static
outcome::std_result<int64_t> Task::ChildTasksCount(
    Database* db,
    Task::Id task_id) noexcept {
  static const std::string_view kQuery =
      "SELECT count(*) FROM Tasks WHERE parent_task_id = :id";
  const std::unordered_map<std::string, Database::Param> params = {
    {":id", Database::Param(task_id)},
  };
  auto maybe_rows = db->Select(kQuery, params);
  if (!maybe_rows) {
    return maybe_rows.error();
  }
  SelectRows& rows = maybe_rows.value();
  const auto next_outcome = rows.NextRow();
  VERIFY(next_outcome != SelectRows::kOutcomeDone);
  if (!next_outcome) {
    return next_outcome.error();
  }
  const std::optional<int64_t> count = rows.Int64Column(0);
  VERIFY(count);
  return *count;
}

// static
outcome::std_result<std::vector<Task>> Task::LoadWithQuery(
    Database* db, std::string_view query,
    const std::unordered_map<std::string, Database::Param>& params) noexcept {
  auto maybe_rows = db->Select(query, params);
  if (!maybe_rows) {
    return maybe_rows.error();
  }
  SelectRows& rows = maybe_rows.value();
  std::vector<Task> result;
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
outcome::std_result<void> Task::Save(Database* db) noexcept {
  if (parent_task_id_ && !is_archived_) {
    // Check, if parent is archived, this task also must be archived.
    const outcome::std_result<Task> maybe_parent = LoadById(
        db, *parent_task_id_);
    if (!maybe_parent) {
      return maybe_parent.error();
    }
    if (maybe_parent.value().is_archived_) {
      is_archived_ = true;
    }
  }
  if (id_) {
    const std::unordered_map<std::string, Database::Param> params = {
      {":name", Database::Param(name_)},
      {":is_archived", Database::Param(is_archived_)},
      {":parent_task_id", Database::Param(parent_task_id_)},
      {":id", Database::Param(id_)},
    };
    auto exec_result = db->Execute(
        "UPDATE Tasks SET "
        " name=:name,"
        " is_archived=:is_archived,"
        " parent_task_id=:parent_task_id"
        " WHERE id=:id",
        params);
    if (!exec_result) {
      return exec_result.error();
    }
  } else {
    const std::unordered_map<std::string, Database::Param> params = {
      {":name", Database::Param(name_)},
      {":is_archived", Database::Param(is_archived_)},
      {":parent_task_id", Database::Param(parent_task_id_)},
    };
    auto exec_result = db->Execute(
        "INSERT INTO Tasks(name, is_archived, parent_task_id) VALUES("
        " :name,"
        " :is_archived,"
        " :parent_task_id"
        ")",
        params);
    if (!exec_result) {
      return exec_result.error();
    }
    id_ = exec_result.value();
  }
  return outcome::success();
}

}  // namespace m_time_tracker
