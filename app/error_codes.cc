// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/error_codes.h"
#include "app/verify.h"

namespace m_time_tracker {

const char* CustomErrorCategory::name() const noexcept {
  return "MobileTimeTrackerError";
}

// Return what each enum means in text
std::string CustomErrorCategory::message(int c) const {
  switch (static_cast<ErrorCodes>(c)) {
    case ErrorCodes::kOk:
      return "No error";
    case ErrorCodes::kUnknownDbParameterName:
      return "Unknown DB parameter name";
    case ErrorCodes::kEmptyResults:
      return "Result set is empty";
    default:
      NOTREACHED();
  }
}

// static
const CustomErrorCategory& CustomErrorCategory::instance() {
  static CustomErrorCategory c;
  return c;
}

const char* SqliteErrorCategory::name() const noexcept {
  return "Sqlite database error";
}

std::string SqliteErrorCategory::message(int c) const {
  return std::string(sqlite3_errstr(c));
}

// static
const SqliteErrorCategory& SqliteErrorCategory::instance() {
  static SqliteErrorCategory c;
  return c;
}

std::error_code make_error_code(ErrorCodes e) {
  return {static_cast<int>(e), CustomErrorCategory::instance()};
}

std::error_code make_error_code(SqliteErrorCodes e) {
  return {static_cast<int>(e), SqliteErrorCategory::instance()};
}

}  // namespace m_time_tracker
