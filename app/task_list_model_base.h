// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include "app/task.h"
#include "app/list_model_base.h"

namespace m_time_tracker {

class AppState;

class TaskListModelBase: public ListModelBase<Task> {
 protected:
  explicit TaskListModelBase(AppState* app_state) noexcept;
};

}  // namespace m_time_tracker
