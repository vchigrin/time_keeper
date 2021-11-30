// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include "app/activity.h"

namespace m_time_tracker {

enum class FormatMode {
  kShortWithSeconds,
  kLongWithoutSeconds,
};

std::string FormatRuntime(
    Activity::Duration runtime, FormatMode mode) noexcept;
std::string FormatTimePoint(Activity::TimePoint time_point) noexcept;

}  // namespace m_time_tracker
