// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <sqlite3.h>

#include <optional>
#include <string>

#include "app/error_codes.h"
#include "app/verify.h"

namespace m_time_tracker {

class SelectRows {
 public:
  static const outcome::std_result<void> kOutcomeDone;

  explicit SelectRows(sqlite3_stmt* stmt) noexcept
      : stmt_(stmt) {
    VERIFY(stmt);
  }
  SelectRows(const SelectRows&) = delete;
  SelectRows& operator = (const SelectRows&) = delete;
  SelectRows(SelectRows&& second) noexcept
      : stmt_(second.stmt_) {
    second.stmt_ = nullptr;
  }

  ~SelectRows();

  outcome::std_result<void> NextRow() noexcept;

  // Column indices are zero-based.
  // Returns nullopt if corresponding cell is NULL.
  std::optional<int> IntColumn(int index) noexcept;
  std::optional<int64_t> Int64Column(int index) noexcept;
  std::optional<std::string> StringColumn(int index) noexcept;

 private:
  sqlite3_stmt* stmt_;
};

}  // namespace m_time_tracker
