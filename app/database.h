// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <sqlite3.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_map>
#include <variant>

#include "app/error_codes.h"
#include "app/select_rows.h"
#include "app/verify.h"

namespace m_time_tracker {

class SelectRows;

// NOTE: At present this class it NOT thread safe.
// To make it so you need to update Execute() function logic.
class Database {
 public:
  class Param {
   public:
    explicit Param(std::string v) noexcept
        : value_(std::move(v)) {}

    explicit Param(int v) noexcept
        : value_(v) {}

    explicit Param(int64_t v) noexcept
        : value_(v) {}

    explicit Param(std::optional<std::string> v) noexcept {
      if (v) {
        value_ = std::move(*v);
      } else {
        value_ = NullTag{};
      }
    }

    explicit Param(std::optional<int> v) noexcept {
      if (v) {
        value_ = *v;
      } else {
        value_ = NullTag{};
      }
    }

    explicit Param(std::optional<int64_t> v) noexcept {
      if (v) {
        value_ = *v;
      } else {
        value_ = NullTag{};
      }
    }

    // NOTE: Param instance must be kept alive till statement is finalized,
    // since it uses SQLITE_STATIC bind mode for strings.
    std::error_code Bind(sqlite3_stmt*, int index) const noexcept;

   private:
    struct NullTag {};
    std::variant<int, int64_t, std::string, NullTag> value_;
  };
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

  // Returns last insert rowid.
  outcome::std_result<int64_t> Execute(
      const std::string_view query,
      const std::unordered_map<std::string, Param>& params) noexcept;

 private:
  explicit Database(sqlite3* connection) noexcept;
  static std::error_code EnsureTablesCreated(sqlite3* connection) noexcept;

  sqlite3* connection_ = nullptr;
};

}  // namespace m_time_tracker
