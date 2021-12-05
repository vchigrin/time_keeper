// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once
#include "app/activity.h"
#include "app/app_state.h"
#include "app/ui_helpers.h"

namespace m_time_tracker {

class MainWindow;

class StatisticsView {
 public:
  StatisticsView(
      MainWindow* main_window,
      const Glib::RefPtr<Gtk::Builder>& resource_builder,
      AppState* app_state) noexcept;
  void Recalculate() noexcept;

 private:
  void InitializeWidgetPointers(
      const Glib::RefPtr<Gtk::Builder>& builder) noexcept;

  // Only date component is edited, time preserved.
  void EditDate(Activity::TimePoint* timepoint) noexcept;
  bool StatisticsDraw(const Cairo::RefPtr<Cairo::Context>& ctx) noexcept;

  Gtk::Button* btn_to_ = nullptr;
  Gtk::Button* btn_from_ = nullptr;
  Gtk::DrawingArea* drawing_ = nullptr;
  Activity::TimePoint to_time_;
  Activity::TimePoint from_time_;
  MainWindow* const main_window_;
  Glib::RefPtr<Gtk::Builder> resource_builder_;
  AppState* const app_state_;
};

}  // namespace m_time_tracker
