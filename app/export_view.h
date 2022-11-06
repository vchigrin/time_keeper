// Copyright 2022 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "app/activity.h"
#include "app/app_state.h"
#include "app/ui_helpers.h"
#include "app/view_with_date_range.h"

namespace m_time_tracker {

class MainWindow;

class ExportView : public ViewWithDateRange {
 public:
  ExportView(
      MainWindow* main_window,
      const Glib::RefPtr<Gtk::Builder>& resource_builder,
      AppState* app_state) noexcept;

 private:
  void OnDateRangeChanged() noexcept override;
  void OnBtnSelectFileClicked() noexcept;
  void OnBtnExportClicked() noexcept;

  void InitializeWidgetPointers(
      const Glib::RefPtr<Gtk::Builder>& builder) noexcept;

  Gtk::Button* btn_select_file_ = nullptr;
  Gtk::Button* btn_export_run_ = nullptr;
  Gtk::Label* lbl_export_file_path_ = nullptr;

  MainWindow* const main_window_;
  Glib::RefPtr<Gtk::Builder> resource_builder_;
  AppState* const app_state_;
  std::string export_file_path_;
};

}  // namespace m_time_tracker
