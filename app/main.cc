// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/ui_helpers.h"
#include "app/main_window.h"

namespace {

void OnStartup() {
  hdy_init();
}

}  // namespace

int main(int argc, char* argv[]) {
  auto app = Gtk::Application::create("org.mobile_time_tracker.app");
  app->signal_startup().connect(&OnStartup);

  Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource(
      "/org/mobile_time_tracker/app/main_window.ui");
  VERIFY(builder);

  std::unique_ptr<m_time_tracker::MainWindow> wnd =
      m_time_tracker::GetWindowDerived<m_time_tracker::MainWindow>(
          builder, "main_window");

  return app->run(*wnd, argc, argv);
}
