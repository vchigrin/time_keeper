// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/utils.h"

#include <boost/format.hpp>

namespace m_time_tracker {

std::string FormatRuntime(Activity::Duration runtime) noexcept {
  std::string result;
  if (runtime >= std::chrono::hours(1)) {
    const auto hours = std::chrono::floor<std::chrono::hours>(runtime);
    result += std::to_string(hours.count());
    result += ".";
    runtime -= hours;
  }

  const auto minutes = std::chrono::floor<std::chrono::minutes>(runtime);
  const auto seconds =
      std::chrono::floor<std::chrono::seconds>(runtime) %
          std::chrono::minutes(1);
  return result + (boost::format("%1$02d:%2$02d") %
      minutes.count() % seconds.count()).str();
}

}  // namespace m_time_tracker
