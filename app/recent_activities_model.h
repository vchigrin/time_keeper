// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include "app/activity.h"
#include "app/list_model_base.h"

namespace m_time_tracker {

class AppState;

class RecentActivitiesModel: public ListModelBase<Activity> {
 protected:
  friend class ListModelBase<Activity>;
  explicit RecentActivitiesModel(AppState* app_state) noexcept;
  Glib::RefPtr<Gtk::Widget> CreateRowFromObject(
      const Activity& a) noexcept override;

 protected:
  Activity::TimePoint earliest_start_time_;
};

}  // namespace m_time_tracker
