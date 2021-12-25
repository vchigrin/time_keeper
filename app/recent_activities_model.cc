// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/recent_activities_model.h"

#include "app/app_state.h"

namespace m_time_tracker {

RecentActivitiesModel::RecentActivitiesModel(
    AppState* app_state,
    Gtk::Window* parent_window,
    Glib::RefPtr<Gtk::Builder> resource_builder) noexcept
    : ActivitiesListModelBase(app_state, parent_window, resource_builder),
      earliest_start_time_(Activity::GetCurrentTimePoint() -
          std::chrono::hours(24)) {
  auto maybe_recent = Activity::LoadAfter(
      &app_state->db_for_read_only(),
      earliest_start_time_);
  VERIFY(maybe_recent);
  SetContent(maybe_recent.value());
}

bool RecentActivitiesModel::ShouldShowActivity(
    const Activity& a) noexcept {
  return a.start_time() >= earliest_start_time_;
}

}  // namespace m_time_tracker
