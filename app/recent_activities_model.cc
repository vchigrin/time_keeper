// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/recent_activities_model.h"

#include "app/app_state.h"
#include "app/main_window.h"

namespace m_time_tracker {

RecentActivitiesModel::RecentActivitiesModel(
    AppState* app_state,
    MainWindow* main_window,
    Glib::RefPtr<Gtk::Builder> resource_builder) noexcept
    : ActivitiesListModelBase(
          app_state, main_window, main_window, resource_builder) {
  Recalculate();
}

bool RecentActivitiesModel::ShouldShowActivity(
    const Activity& a) noexcept {
  return a.start_time() >= earliest_start_time_;
}

void RecentActivitiesModel::Recalculate() noexcept {
  earliest_start_time_ = Activity::GetCurrentTimePoint() -
          std::chrono::hours(24);
  auto maybe_recent = Activity::LoadAfter(
      &app_state_->db_for_read_only(),
      earliest_start_time_);
  VERIFY(maybe_recent);
  SetContent(maybe_recent.value());
}

}  // namespace m_time_tracker
