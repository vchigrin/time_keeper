// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/filtered_activities_dialog.h"

namespace m_time_tracker {

FilteredActivitiesDialog::FilteredActivitiesDialog(
    GtkDialog* dlg,
    const Glib::RefPtr<Gtk::Builder>& builder,
    AppState* app_state,
    MainWindow* main_window) noexcept
    : Gtk::Dialog(dlg) {
  Gtk::ListBox* lst_filtered_activities =
      GetWidgetChecked<Gtk::ListBox>(builder, "lst_filtered_activities");
  activities_model_ = ActivitiesListModelBase::create<ActivitiesListModelBase>(
      app_state, main_window, this, builder);
  lst_filtered_activities->bind_model(
      activities_model_,
      activities_model_->slot_create_widget());
}

}  // namespace m_time_tracker
