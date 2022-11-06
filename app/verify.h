// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once
#include <string_view>

namespace m_time_tracker {
// Few macros to always abort program in case any error condition encountered.

[[noreturn]]
void HandleAssertionFailure(
    const std::string_view expr,
    const std::string_view file,
    int line) noexcept;

#define VERIFY(expr) \
  if (!(expr)) { \
    ::m_time_tracker::HandleAssertionFailure(#expr, __FILE__, __LINE__); \
  }

#define NOTREACHED() VERIFY(false)

}  // namespace m_time_tracker
