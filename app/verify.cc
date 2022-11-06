// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/verify.h"

#include <cstdlib>
#include <iostream>

namespace m_time_tracker {

void HandleAssertionFailure(
    const std::string_view expr,
    const std::string_view file,
    int line) noexcept {
  std::cerr << "Assertion " << expr
            << " failed at "
            << file << ":" << line
            << std::endl;
  std::abort();
}

}  // namespace m_time_tracker
