// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once
#include <cstdlib>

// Few macros to always abort program in case any error condition encountered.

#define VERIFY(expr) \
  if (!(expr)) { \
    std::abort(); \
  }

#define NOTREACHED() std::abort();
