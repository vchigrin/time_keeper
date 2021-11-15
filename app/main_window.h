// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <optional>

#include "app/ui_helpers.h"
#include "app/database.h"

namespace m_time_tracker {

class EditTaskDialog;

class MainWindow : public Gtk::Window {
 public:
  MainWindow(GtkWindow* wnd, const Glib::RefPtr<Gtk::Builder>& builder);
  ~MainWindow();

 private:
  void InitializeWidgetPointers(
      const Glib::RefPtr<Gtk::Builder>& builder) noexcept;

  void OnBtnMenuClicked() noexcept;
  void OnBtnNewTaskClicked() noexcept;
  void OnPageStackVisibleChildChanged() noexcept;
  void RefreshTasksList() noexcept;

  Gtk::Button* btn_menu_ = nullptr;
  Gtk::Button* btn_new_task_ = nullptr;
  Gtk::Stack* main_stack_ = nullptr;
  Gtk::Stack* page_stack_ = nullptr;
  Gtk::Label* lbl_running_time_ = nullptr;
  Gtk::StackSidebar* page_stack_sidebar_ = nullptr;
  Glib::RefPtr<Gtk::Builder> resource_builder_;
  std::optional<Database> db_;
  std::unique_ptr<EditTaskDialog> edit_task_dialog_;
};

}  // namespace m_time_tracker
