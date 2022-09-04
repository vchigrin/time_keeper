// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include "app/activity.h"
#include "app/activities_list_model_base.h"

namespace m_time_tracker {

class AppState;
class MainWindow;

class RecentActivitiesModel: public ActivitiesListModelBase {
 protected:
  friend class ListModelBase<Activity>;
  RecentActivitiesModel(
      AppState* app_state,
      MainWindow* main_window,
      Glib::RefPtr<Gtk::Builder> resource_builder) noexcept;

 private:
  bool ShouldShowActivity(const Activity& a) noexcept override;

  Activity::TimePoint earliest_start_time_;
};

}  // namespace m_time_tracker
