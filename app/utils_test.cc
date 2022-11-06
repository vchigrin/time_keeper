// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "app/utils.h"

using m_time_tracker::FormatRuntime;
using m_time_tracker::FormatMode;

TEST(UtilsTest, TaskSave) {
  const auto mode_short = FormatMode::kShortWithSeconds;

  EXPECT_EQ("00:00", FormatRuntime(std::chrono::seconds(0), mode_short));
  EXPECT_EQ("00:01", FormatRuntime(std::chrono::seconds(1), mode_short));
  EXPECT_EQ("01:00", FormatRuntime(std::chrono::seconds(60), mode_short));
  EXPECT_EQ("01:01", FormatRuntime(std::chrono::seconds(61), mode_short));
  EXPECT_EQ("1.00:00", FormatRuntime(std::chrono::seconds(3600), mode_short));
  EXPECT_EQ("1.00:01", FormatRuntime(std::chrono::seconds(3601), mode_short));
  EXPECT_EQ("1.01:01", FormatRuntime(std::chrono::seconds(3661), mode_short));
  EXPECT_EQ("1.10:01", FormatRuntime(std::chrono::seconds(4201), mode_short));
}
