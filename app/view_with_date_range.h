// Copyright 2022 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include "app/app_state.h"
#include "app/ui_helpers.h"

namespace m_time_tracker {

class MainWindow;

class ViewWithDateRange {
 public:
  ViewWithDateRange(
      MainWindow* main_window,
      const Glib::RefPtr<Gtk::Builder>& resource_builder,
      AppState* app_state,
      const Glib::ustring& btn_stat_from_name,
      const Glib::ustring& btn_stat_to_name,
      const Glib::ustring& cmb_quick_select_name) noexcept;
  virtual ~ViewWithDateRange() = default;

 protected:
  Activity::TimePoint to_time() const noexcept {
    return to_time_;
  }

  Activity::TimePoint from_time() const noexcept {
    return from_time_;
  }

  virtual void OnDateRangeChanged() noexcept = 0;

 private:
  void InitializeWidgetPointers(
      const Glib::RefPtr<Gtk::Builder>& builder,
      const Glib::ustring& btn_stat_from_name,
      const Glib::ustring& btn_stat_to_name,
      const Glib::ustring& cmb_quick_select_name) noexcept;

  // Only date component is edited, time preserved.
  void EditDate(Activity::TimePoint* timepoint) noexcept;

  void OnComboQuickSelectChanged() noexcept;

  Gtk::Button* btn_to_ = nullptr;
  Gtk::Button* btn_from_ = nullptr;
  Gtk::ComboBoxText* cmb_quick_select_ = nullptr;
  Activity::TimePoint to_time_;
  Activity::TimePoint from_time_;
  MainWindow* const main_window_;
  Glib::RefPtr<Gtk::Builder> resource_builder_;
  AppState* const app_state_;
};

}  // namespace m_time_tracker
