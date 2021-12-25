// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once
#include <vector>

#include "app/activity.h"
#include "app/activities_list_model_base.h"

namespace m_time_tracker {

class AppState;

class FilteredActivitiesDialog : public Gtk::Dialog {
 public:
  FilteredActivitiesDialog(
      GtkDialog* dlg,
      const Glib::RefPtr<Gtk::Builder>& builder,
      AppState* app_state);

  void SetActivitiesList(const std::vector<Activity>& activities) noexcept {
    activities_model_->SetContent(activities);
  }

 private:
  Glib::RefPtr<ActivitiesListModelBase> activities_model_;
};

}  // namespace m_time_tracker
