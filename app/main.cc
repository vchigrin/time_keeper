// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/ui_helpers.h"
#include "app/main_window.h"
#include "app/db_wrapper.h"

namespace {

void OnStartup() {
  hdy_init();
  Glib::RefPtr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create();
  css_provider->load_from_resource("/org/mobile_time_tracker/app/style.css");
  Gtk::StyleContext::add_provider_for_screen(
      Gdk::Screen::get_default(),
      css_provider,
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

}  // namespace

int main(int argc, char* argv[]) {
  auto app = Gtk::Application::create("org.mobile_time_tracker.app");
  app->signal_startup().connect(&OnStartup);

  Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource(
      "/org/mobile_time_tracker/app/main_window.ui");
  VERIFY(builder);

  // TODO(vchigrin): Save DB in permanent location.
  auto maybe_db_wrapper = m_time_tracker::DbWrapper::Open(":memory:");
  // TODO(vchigrin): Proper error handling
  VERIFY(maybe_db_wrapper);

  m_time_tracker::DbWrapper db_wrapper(std::move(maybe_db_wrapper.value()));

  std::unique_ptr<m_time_tracker::MainWindow> wnd =
      m_time_tracker::GetWindowDerived<m_time_tracker::MainWindow>(
          builder, "main_window",
          &db_wrapper);

  return app->run(*wnd, argc, argv);
}
