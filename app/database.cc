// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/database.h"

#include "app/error_codes.h"
#include "app/select_rows.h"
#include "app/verify.h"

namespace m_time_tracker {

namespace {

int EmptyCallback(
    void*, int /* num_columns */,
    char** /* column_values */,
    char** /* column_names */) {
  return SQLITE_OK;
}

}  // namespace

// static
outcome::std_result<Database> Database::Open(
    const std::filesystem::path& db_path) noexcept {
  sqlite3* connection = nullptr;
  const int result = sqlite3_open(db_path.native().c_str(), &connection);
  if (result != SQLITE_OK) {
    return ErrorCodeFromSqlite(result);
  }
  const auto create_error = Database::EnsureTablesCreated(connection);
  if (create_error) {
    return create_error;
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

// static
std::error_code Database::EnsureTablesCreated(sqlite3* connection) noexcept {
  char* error_ptr = nullptr;
  const int result = sqlite3_exec(
      connection,
      "CREATE TABLE IF NOT EXISTS Tasks( "
      "  id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "  name TEXT NOT NULL, "
      "  parent_task_id INTEGER, "
      "  is_archived INTEGER) ",
      &EmptyCallback,
      nullptr,
      &error_ptr);
  if (result != SQLITE_OK) {
    return ErrorCodeFromSqlite(result);
  }
  return ErrorCodeFromSqlite(SQLITE_OK);
}

outcome::std_result<SelectRows> Database::Select(
    std::string_view query) noexcept {
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
  return SelectRows(stmt);
}

}  // namespace m_time_tracker
