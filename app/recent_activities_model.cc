// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/recent_activities_model.h"

#include <string>

#include <boost/format.hpp>

#include "app/app_state.h"
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

RecentActivitiesModel::RecentActivitiesModel(
    AppState* app_state, Gtk::Window* parent_window) noexcept
    : ListModelBase(app_state),
      earliest_start_time_(Activity::GetCurrentTimePoint() -
          std::chrono::hours(24)),
      parent_window_(parent_window) {
  VERIFY(parent_window_);
  all_connections_.emplace_back(app_state->ConnectExistingActivityChanged(
      sigc::mem_fun(*this, &RecentActivitiesModel::ExistingObjectChanged)));
  all_connections_.emplace_back(app_state->ConnectAfterActivityAdded(
      sigc::mem_fun(*this, &RecentActivitiesModel::AfterObjectAdded)));
  all_connections_.emplace_back(app_state->ConnectBeforeActivityDeleted(
      sigc::mem_fun(*this, &RecentActivitiesModel::BeforeObjectDeleted)));

  auto maybe_recent = Activity::LoadAfter(
      &app_state->db_for_read_only(),
      earliest_start_time_);
  VERIFY(maybe_recent);
  Initialize(maybe_recent.value());
}

Glib::RefPtr<Gtk::Widget> RecentActivitiesModel::CreateRowFromObject(
    const Activity& a) noexcept {
  if (a.start_time() < earliest_start_time_) {
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
    // TODO(vchigrin): Implement.
    // EditActivity(activity_id);
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

void RecentActivitiesModel::DeleteActivity(Activity::Id activity_id) noexcept {
  auto maybe_activity = Activity::LoadById(
      &app_state_->db_for_read_only(), activity_id);
  VERIFY(maybe_activity);
  auto maybe_task = Task::LoadById(
      &app_state_->db_for_read_only(), maybe_activity.value().task_id());
  VERIFY(maybe_task);
  // TODO(vchigrin): Localization.
  const std::string message =
      (boost::format("Delete activity for task \"%1%\"?") %
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
    // TODO(vchigrin): Better error handling.
    VERIFY(delete_result);
  }
}

}  // namespace m_time_tracker
