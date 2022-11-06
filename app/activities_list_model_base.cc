// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/activities_list_model_base.h"

#include <string>
#include <utility>

#include <boost/format.hpp>

#include "app/app_state.h"
#include "app/main_window.h"
#include "app/utils.h"

namespace m_time_tracker {

namespace {

std::string CreateSubtitle(const Activity& a) {
  VERIFY(a.end_time());
  return (boost::format("%1% - %2%") %
      FormatTimePoint(a.start_time()) %
      FormatTimePoint(*a.end_time())).str();
}

}  // namespace

ActivitiesListModelBase::ActivitiesListModelBase(
    AppState* app_state,
    MainWindow* main_window,
    Gtk::Window* parent_window,
    Glib::RefPtr<Gtk::Builder> resource_builder,
    bool show_recent_first) noexcept
    : ListModelBase(app_state),
      main_window_(main_window),
      parent_window_(parent_window),
      resource_builder_(std::move(resource_builder)),
      show_recent_first_(show_recent_first) {
  VERIFY(main_window_);
  all_connections_.emplace_back(app_state->ConnectExistingActivityChanged(
      sigc::mem_fun(*this, &ActivitiesListModelBase::ExistingObjectChanged)));
  all_connections_.emplace_back(app_state->ConnectAfterActivityAdded(
      sigc::mem_fun(*this, &ActivitiesListModelBase::AfterObjectAdded)));
  all_connections_.emplace_back(app_state->ConnectBeforeActivityDeleted(
      sigc::mem_fun(*this, &ActivitiesListModelBase::BeforeObjectDeleted)));
}

Glib::RefPtr<Gtk::Widget> ActivitiesListModelBase::CreateRowFromObject(
    const Activity& a) noexcept {
  if (!ShouldShowActivity(a)) {
    return Glib::RefPtr<Gtk::Widget>();
  }
  GtkListBoxRow* row = reinterpret_cast<GtkListBoxRow*>(hdy_action_row_new());
  g_object_ref_sink(row);
  Glib::RefPtr<Gtk::ListBoxRow> wrapped_row(Glib::wrap(row));

  auto load_result = Task::LoadById(
      &app_state_->db_for_read_only(),
      a.task_id());
  VERIFY(load_result);
  const Task& t = load_result.value();

  const std::string& title = t.name();
  hdy_preferences_row_set_title(
      reinterpret_cast<HdyPreferencesRow*>(wrapped_row->gobj()),
      title.c_str());
  const std::string subtitle = CreateSubtitle(a);
  hdy_action_row_set_subtitle(
      reinterpret_cast<HdyActionRow*>(row),
      subtitle.c_str());

  const auto duration = *a.end_time() - a.start_time();
  const std::string runtime = FormatRuntime(
      duration, FormatMode::kLongWithoutSeconds);
  Gtk::Label* label = manage(new Gtk::Label(runtime));
  wrapped_row->add(*label);
  label->show();

  Glib::RefPtr<Gtk::Button> btn_edit(new Gtk::Button());
  btn_edit->set_image_from_icon_name("gtk-edit");
  btn_edit->show();
  btn_edit->signal_clicked().connect([this, activity_id = *a.id()]() {
    EditActivity(activity_id);
  });
  wrapped_row->add(*btn_edit.get());

  Glib::RefPtr<Gtk::Button> btn_delete(new Gtk::Button());
  btn_delete->set_image_from_icon_name("edit-delete");
  btn_delete->show();
  btn_delete->signal_clicked().connect([this, activity_id = *a.id()]() {
    DeleteActivity(activity_id);
  });
  wrapped_row->add(*btn_delete.get());

  wrapped_row->show();
  return wrapped_row;
}

void ActivitiesListModelBase::DeleteActivity(
    Activity::Id activity_id) noexcept {
  auto maybe_activity = Activity::LoadById(
      &app_state_->db_for_read_only(), activity_id);
  VERIFY(maybe_activity);
  auto maybe_task = Task::LoadById(
      &app_state_->db_for_read_only(), maybe_activity.value().task_id());
  VERIFY(maybe_task);
  const std::string message =
      (boost::format(_L("Delete activity for task \"%1%\"?")) %
          maybe_task.value().name()).str();
  Gtk::MessageDialog message_dlg(
      *parent_window_,
      message,
      /* use_markup */ false,
      Gtk::MESSAGE_QUESTION,
      Gtk::BUTTONS_YES_NO,
      /* modal */ true);
  if (message_dlg.run() == Gtk::RESPONSE_YES) {
    auto delete_result = app_state_->DeleteActivity(maybe_activity.value());
    if (!delete_result) {
      main_window_->OnFatalError(delete_result.assume_error());
    }
  }
}

void ActivitiesListModelBase::EditActivity(Activity::Id activity_id) noexcept {
  auto maybe_activity = Activity::LoadById(
      &app_state_->db_for_read_only(), activity_id);
  VERIFY(maybe_activity);
  Activity& activity = maybe_activity.value();
  Glib::RefPtr<EditActivityDialog> edit_activity_dialog =
      GetWindowDerived<EditActivityDialog>(
          resource_builder_, "edit_activity_dialog", app_state_,
          main_window_);
  edit_activity_dialog->set_activity(&activity);
  while (true) {
    const int result = edit_activity_dialog->run();
    if (result != Gtk::RESPONSE_OK) {
      break;
    }
    if (activity.end_time() <= activity.start_time()) {
      Gtk::MessageDialog message_dlg(
          *edit_activity_dialog.get(),
          _L("Error - end time must be after start time."),
          /* use_markup */ false,
          Gtk::MESSAGE_ERROR,
          Gtk::BUTTONS_OK,
          /* modal */ true);
      message_dlg.run();
      continue;
    }
    auto save_result = app_state_->SaveChangedActivity(&activity);
    if (!save_result) {
      main_window_->OnFatalError(save_result.assume_error());
    }
    break;
  }
  // Ensure we'll never produce dangling pointers. Note, that dialog object
  // may re reused since Gtk::Builder holds reference to it.
  edit_activity_dialog->set_activity(nullptr);
  edit_activity_dialog->hide();
}

}  // namespace m_time_tracker
