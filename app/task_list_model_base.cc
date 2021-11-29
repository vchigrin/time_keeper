// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/task_list_model_base.h"

#include <utility>

#include "app/app_state.h"

namespace m_time_tracker {

TaskListModelBase::TaskListModelBase(AppState* app_state) noexcept
    : ListModelBase(app_state) {
  all_connections_.emplace_back(app_state->ConnectExistingTaskChanged(
      sigc::mem_fun(*this, &TaskListModelBase::ExistingObjectChanged)));
  all_connections_.emplace_back(app_state->ConnectAfterTaskAdded(
      sigc::mem_fun(*this, &TaskListModelBase::AfterObjectAdded)));
  all_connections_.emplace_back(app_state->ConnectBeforeTaskDeleted(
      sigc::mem_fun(*this, &TaskListModelBase::BeforeObjectDeleted)));
}

}  // namespace m_time_tracker
