// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/utils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/format.hpp>

#include "app/verify.h"

namespace m_time_tracker {

std::string FormatRuntime(
    Activity::Duration runtime, FormatMode mode) noexcept {
  std::string result;
  if (runtime >= std::chrono::hours(1)) {
    const auto hours = std::chrono::floor<std::chrono::hours>(runtime);
    result += std::to_string(hours.count());
    switch (mode) {
      case FormatMode::kShortWithSeconds:
        result += ".";
        break;
      case FormatMode::kLongWithoutSeconds:
        result += _L(" hours ");
        break;
    }

    runtime -= hours;
  }

  const auto minutes = std::chrono::floor<std::chrono::minutes>(runtime);
  const auto seconds =
      std::chrono::floor<std::chrono::seconds>(runtime) %
          std::chrono::minutes(1);

  switch (mode) {
    case FormatMode::kShortWithSeconds:
      return result + (boost::format("%1$02d:%2$02d") %
          minutes.count() % seconds.count()).str();
    case FormatMode::kLongWithoutSeconds:
      return result + (boost::format(_L("%1% min")) % minutes.count()).str();
    default:
      NOTREACHED();
  }
}

std::tm TimePointToLocal(Activity::TimePoint time_point) noexcept {
  const std::time_t tp = std::chrono::system_clock::to_time_t(time_point);
  const std::tm* local_time = std::localtime(&tp);
  VERIFY(local_time);
  return *local_time;
}

Activity::TimePoint TimePointFromLocal(std::tm local_time) noexcept {
  const std::time_t tp = std::mktime(&local_time);
  VERIFY(tp != -1);
  return std::chrono::floor<Activity::Duration>(
      std::chrono::system_clock::from_time_t(tp));
}

std::string FormatTimePoint(Activity::TimePoint time_point) noexcept {
  const std::tm local_time = TimePointToLocal(time_point);
  std::stringstream sstrm;
  sstrm << std::put_time(&local_time, "%b %d %H:%M");
  return sstrm.str();
}

Activity::TimePoint GetLocalEndDayTimepoint(
    Activity::TimePoint reference) noexcept {
  std::tm local_time = TimePointToLocal(reference);
  local_time.tm_hour = 23;
  local_time.tm_min = 59;
  local_time.tm_sec = 59;
  return TimePointFromLocal(local_time);
}

Activity::TimePoint GetLocalStartDayTimepoint(
    Activity::TimePoint reference) noexcept {
  std::tm local_time = TimePointToLocal(reference);
  local_time.tm_hour = 0;
  local_time.tm_min = 0;
  local_time.tm_sec = 0;
  return TimePointFromLocal(local_time);
}

}  // namespace m_time_tracker
