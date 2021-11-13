// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <sqlite3.h>

#include <filesystem>
#include <string_view>

#include "app/error_codes.h"
#include "app/select_rows.h"
#include "app/verify.h"

namespace m_time_tracker {

class SelectRows;

class Database {
 public:
  static outcome::std_result<Database> Open(
      const std::filesystem::path& db_path) noexcept;
  ~Database();

  Database(const Database&) = delete;
  Database(Database&& second)
     : connection_(second.connection_) {
    second.connection_ = nullptr;
  }

  Database& operator = (const Database&) = delete;

  // Returned SelectRows is on position "before first" row.
  // Client should call |NextRow()| at least once on it.
  outcome::std_result<SelectRows> Select(std::string_view query) noexcept;

 private:
  explicit Database(sqlite3* connection) noexcept;
  static std::error_code EnsureTablesCreated(sqlite3* connection) noexcept;

  sqlite3* connection_ = nullptr;
};

}  // namespace m_time_tracker
