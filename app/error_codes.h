// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once
#include <sqlite3.h>

#include <system_error>
#include <string>
#include <type_traits>

#include <boost/outcome/outcome.hpp>

namespace m_time_tracker {

enum class ErrorCodes {
  kOk = 0,
  kUnknownDbParameterName,
  kEmptyResults,
};

static_assert(SQLITE_OK == 0, "Ok code should be 0 for std::result");

enum SqliteErrorCodes : int {
  kOk = SQLITE_OK,
};

class CustomErrorCategory : public std::error_category {
 public:
  const char* name() const noexcept override;
  std::string message(int c) const override;

  static const CustomErrorCategory& instance();
};

class SqliteErrorCategory : public std::error_category {
 public:
  const char* name() const noexcept override;
  std::string message(int c) const override;

  static const SqliteErrorCategory& instance();
};

std::error_code make_error_code(ErrorCodes e);
std::error_code make_error_code(SqliteErrorCodes e);

inline std::error_code ErrorCodeFromSqlite(int sqlite_error) noexcept {
  return make_error_code(static_cast<SqliteErrorCodes>(sqlite_error));
}

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;

}  // namespace m_time_tracker

namespace std {

template <> struct is_error_code_enum<m_time_tracker::ErrorCodes>
    : true_type { };
template <> struct is_error_code_enum<m_time_tracker::SqliteErrorCodes>
    : true_type { };

}  // namespace std

