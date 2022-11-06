// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>

#include "app/ui_helpers.h"
#include "app/main_window.h"
#include "app/app_state.h"

namespace {

constexpr std::string_view kAppFolderName = ".time_keeper";
constexpr std::string_view kDbFileName = "data.dat";

void OnStartup() {
  hdy_init();
  Glib::RefPtr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create();
  css_provider->load_from_resource(
      "/io/github/vchigrin/time_keeper/style.css");
  Gtk::StyleContext::add_provider_for_screen(
      Gdk::Screen::get_default(),
      css_provider,
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

std::filesystem::path GetDbPathOrExit() noexcept {
  const char* home = std::getenv("HOME");
  if (!home) {
    std::cerr << "HOME environment variable is not set" << std::endl;
    std::exit(1);
  }
  const auto app_folder = std::filesystem::path(home) / kAppFolderName;
  const int result = ::mkdir(app_folder.native().c_str(), 0700);
  const bool dir_ok = (result == 0 || (result == -1 && errno == EEXIST));
  if (!dir_ok) {
    std::cerr << "Failed create directory "
              << app_folder.native() << std::endl;
    std::exit(1);
  }
  return app_folder / kDbFileName;
}

}  // namespace

int main(int argc, char* argv[]) {
  bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  auto app = Gtk::Application::create("io.github.vchigrin.time_keeper");
  app->signal_startup().connect(&OnStartup);

  Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource(
      "/io/github/vchigrin/time_keeper/main_window.ui");
  VERIFY(builder);

  const std::filesystem::path db_path = GetDbPathOrExit();

  auto maybe_app_state = m_time_tracker::AppState::Open(db_path.native());
  if (!maybe_app_state) {
    std::cerr << "Failed open database file " << db_path
              << " Error " << maybe_app_state.error().message()
              << std::endl;
    return 1;
  }

  m_time_tracker::AppState app_state(std::move(maybe_app_state.value()));

  Glib::RefPtr<m_time_tracker::MainWindow> wnd =
      m_time_tracker::GetWindowDerived<m_time_tracker::MainWindow>(
          builder, "main_window",
          &app_state);

  return app->run(*wnd.get(), argc, argv);
}
