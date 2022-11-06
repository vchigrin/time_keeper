// Copyright 2022 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/export_view.h"

#include <gtkmm/filechooserdialog.h>

#include <string>
#include <utility>

#include <boost/format.hpp>

#include "app/ui_helpers.h"
#include "app/utils.h"
#include "app/main_window.h"
#include "app/csv_exporter.h"

namespace m_time_tracker {

ExportView::ExportView(
    MainWindow* main_window,
    const Glib::RefPtr<Gtk::Builder>& resource_builder,
    AppState* app_state) noexcept
    : ViewWithDateRange(
        main_window,
        resource_builder,
        app_state,
        "btn_export_from",
        "btn_export_to",
        "cmb_export_quick_select_date"),
      main_window_(main_window),
      resource_builder_(resource_builder),
      app_state_(app_state) {
  InitializeWidgetPointers(resource_builder);

  btn_select_file_->signal_clicked().connect(
      sigc::mem_fun(*this, &ExportView::OnBtnSelectFileClicked));
  btn_export_run_->signal_clicked().connect(
      sigc::mem_fun(*this, &ExportView::OnBtnExportClicked));
  btn_export_run_->set_sensitive(!export_file_path_.empty());
}

void ExportView::InitializeWidgetPointers(
    const Glib::RefPtr<Gtk::Builder>& builder) noexcept {
  btn_select_file_ = GetWidgetChecked<Gtk::Button>(
      builder, "btn_export_file_path");
  btn_export_run_ = GetWidgetChecked<Gtk::Button>(builder, "btn_export_run");
  lbl_export_file_path_ = GetWidgetChecked<Gtk::Label>(
      builder, "lbl_export_file_path");
}

void ExportView::OnDateRangeChanged() noexcept {}

void ExportView::OnBtnSelectFileClicked() noexcept {
  Gtk::FileChooserDialog open_dlg(
      *main_window_,
      _L("Select file"),
      Gtk::FILE_CHOOSER_ACTION_SAVE);
  open_dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  open_dlg.add_button("_Open", Gtk::RESPONSE_OK);

  Glib::RefPtr<Gtk::FileFilter> filter_csv = Gtk::FileFilter::create();
  filter_csv->set_name("CSV files");
  filter_csv->add_pattern("*.csv");
  open_dlg.add_filter(filter_csv);
  if (open_dlg.run() == Gtk::RESPONSE_OK) {
    export_file_path_ = open_dlg.get_filename();
    lbl_export_file_path_->set_label(export_file_path_);
  }
  btn_export_run_->set_sensitive(!export_file_path_.empty());
}

void ExportView::OnBtnExportClicked() noexcept {
  VERIFY(!export_file_path_.empty());
  CSVExporter exporter(
      &app_state_->db_for_read_only(),
      from_time(),
      to_time(),
      export_file_path_);
  const auto export_result = exporter.Run();
  if (export_result) {
    Gtk::MessageDialog message_dlg(
        *main_window_,
        _L("Export completed successfully."),
        /* use_markup */ false,
        Gtk::MESSAGE_INFO,
        Gtk::BUTTONS_OK,
        /* modal */ true);
    message_dlg.run();
  } else {
    const std::string error_message = boost::str(boost::format(
        _L("Export failed. Error \"%1%\".")) %
            export_result.error().message());
    Gtk::MessageDialog message_dlg(
        *main_window_,
        error_message,
        /* use_markup */ false,
        Gtk::MESSAGE_ERROR,
        Gtk::BUTTONS_OK,
        /* modal */ true);
    message_dlg.run();
  }
}

}  // namespace m_time_tracker
