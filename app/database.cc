// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/database.h"

#include "app/error_codes.h"
#include "app/select_rows.h"
#include "app/task.h"
#include "app/verify.h"

namespace m_time_tracker {

// static
outcome::std_result<Database> Database::Open(
    const std::filesystem::path& db_path) noexcept {
  sqlite3* connection = nullptr;
  const int result = sqlite3_open(db_path.native().c_str(), &connection);
  if (result != SQLITE_OK) {
    return ErrorCodeFromSqlite(result);
  }
  return Database(connection);
}

Database::Database(sqlite3* connection) noexcept
    : connection_(connection) {
  VERIFY(connection_);
}

Database::~Database() {
  if (connection_) {
    const int result = sqlite3_close(connection_);
    VERIFY(result == SQLITE_OK);
  }
}

outcome::std_result<SelectRows> Database::Select(
    std::string_view query,
    const std::unordered_map<std::string, Param>& params) noexcept {
  VERIFY(connection_);  // Check that we're not moved-from.
  sqlite3_stmt* stmt = nullptr;
  const int result = sqlite3_prepare_v2(
      connection_,
      query.data(),
      static_cast<int>(query.size()),
      &stmt,
      nullptr);
  if (result != SQLITE_OK) {
    return ErrorCodeFromSqlite(result);
  }
  for (const auto& [key, value] : params) {
    const int index = sqlite3_bind_parameter_index(stmt, key.c_str());
    if (index == 0) {
      const int result = sqlite3_finalize(stmt);
      VERIFY(result == SQLITE_OK);
      return ErrorCodes::kUnknownDbParameterName;
    }
    const outcome::std_result<void> bind_result = value.Bind(stmt, index);
    if (!bind_result) {
      const int result = sqlite3_finalize(stmt);
      VERIFY(result == SQLITE_OK);
      return bind_result.error();
    }
  }

  return SelectRows(stmt);
}

outcome::std_result<int64_t> Database::Execute(
    const std::string_view query,
    const std::unordered_map<std::string, Param>& params) noexcept {
  VERIFY(connection_);  // Check that we're not moved-from.
  sqlite3_stmt* stmt = nullptr;
  const int result = sqlite3_prepare_v2(
      connection_,
      query.data(),
      static_cast<int>(query.size()),
      &stmt,
      nullptr);
  if (result != SQLITE_OK) {
    return ErrorCodeFromSqlite(result);
  }
  for (const auto& [key, value] : params) {
    const int index = sqlite3_bind_parameter_index(stmt, key.c_str());
    if (index == 0) {
      const int result = sqlite3_finalize(stmt);
      VERIFY(result == SQLITE_OK);
      return ErrorCodes::kUnknownDbParameterName;
    }
    const outcome::std_result<void> bind_result = value.Bind(stmt, index);
    if (!bind_result) {
      const int result = sqlite3_finalize(stmt);
      VERIFY(result == SQLITE_OK);
      return bind_result.error();
    }
  }
  const int step_result = sqlite3_step(stmt);
  if (step_result != SQLITE_DONE) {
    return ErrorCodeFromSqlite(result);
  }
  const int find_result = sqlite3_finalize(stmt);
  VERIFY(find_result == SQLITE_OK);
  return sqlite3_last_insert_rowid(connection_);
}

outcome::std_result<void> Database::Param::Bind(
    sqlite3_stmt* stmt, int index) const noexcept {
  const int result = std::visit(
      [stmt, index](const auto& concrete_val) {
        using ParamType = std::decay_t<decltype(concrete_val)>;
        if constexpr (std::is_same_v<int, ParamType>) {
          return sqlite3_bind_int(stmt, index, concrete_val);
        } else if constexpr (std::is_same_v<int64_t, ParamType>) {
          return sqlite3_bind_int64(stmt, index, concrete_val);
        } else if constexpr (std::is_same_v<std::string, ParamType>) {
          return sqlite3_bind_text(
              stmt,
              index,
              concrete_val.c_str(),
              static_cast<int>(concrete_val.size()),
              SQLITE_STATIC);
        } else if constexpr (std::is_same_v<NullTag, ParamType>) {
          return sqlite3_bind_null(stmt, index);
        }
      },
      value_);
  if (result != SQLITE_OK) {
    return ErrorCodeFromSqlite(result);
  } else {
    return outcome::success();
  }
}

}  // namespace m_time_tracker
