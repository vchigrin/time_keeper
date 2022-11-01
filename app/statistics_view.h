// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once
#include <unordered_map>
#include <vector>

#include "app/activity.h"
#include "app/app_state.h"
#include "app/ui_helpers.h"
#include "app/view_with_date_range.h"

namespace m_time_tracker {

class MainWindow;

class StatisticsView : public ViewWithDateRange {
 public:
  StatisticsView(
      MainWindow* main_window,
      const Glib::RefPtr<Gtk::Builder>& resource_builder,
      AppState* app_state) noexcept;
  ~StatisticsView();
  void ResetCurrentTaskAndRecalculate() noexcept;

 private:
  void Recalculate() noexcept;
  struct DisplayedStatInfo {
    Activity::StatEntry stat;
    std::optional<Cairo::RectangleInt> last_drawn_rect;
  };
  std::vector<DisplayedStatInfo> displayed_stats_;

  void OnDateRangeChanged() noexcept override;

  void InitializeWidgetPointers(
      const Glib::RefPtr<Gtk::Builder>& builder) noexcept;

  bool StatisticsDraw(const Cairo::RefPtr<Cairo::Context>& ctx) noexcept;
  bool OnDrawingButtonPressed(GdkEventButton* evt) noexcept;

  void OnExistingTaskChanged(const Task& task) noexcept;
  Cairo::RectangleInt DrawStatEntryRect(
      const Cairo::RefPtr<Cairo::Context>& ctx,
      int control_width,
      double current_y,
      const Glib::RefPtr<Pango::Layout>& pango_layout,
      const Activity::Duration& total_duration,
      const Activity::Duration& task_duration,
      const Task& task) const noexcept;
  void SetFontForBarDrawing(
      const Glib::RefPtr<Pango::Layout>& pango_layout) noexcept;
  int CalculageContentHeight() noexcept;
  bool HasChildren(const Task& task) const noexcept;

  Gtk::DrawingArea* drawing_ = nullptr;
  // If not none, only children of specified parent task is displayed.
  // Else - statistics for top-level tasks are displayed.
  std::optional<Task::Id> current_parent_task_id_;
  MainWindow* const main_window_;
  Glib::RefPtr<Gtk::Builder> resource_builder_;
  AppState* const app_state_;
  std::unordered_map<Task::Id, Task> tasks_cache_;
  sigc::connection existing_task_changed_connection_;
};

}  // namespace m_time_tracker
