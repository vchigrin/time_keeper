// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include "app/edit_task_dialog.h"

#include <charconv>
#include <vector>
#include <boost/algorithm/string/trim.hpp>

#include "app/app_state.h"

namespace m_time_tracker {

// Important - must be distinct from any string that may represent
// Task::Id.
static constexpr char kNoneTaskId[] = "<NONE>";

EditTaskDialog::EditTaskDialog(
    GtkDialog* dlg,
    const Glib::RefPtr<Gtk::Builder>& builder,
    AppState* app_state)
    : Gtk::Dialog(dlg),
      app_state_(app_state) {
  edt_task_name_ = GetWidgetChecked<Gtk::Entry>(builder, "edt_task_name");
  btn_ok_ = GetWidgetChecked<Gtk::Button>(builder, "btn_ok");
  chk_archived_ = GetWidgetChecked<Gtk::CheckButton>(builder, "chk_archived");
  cmb_parent_task_ = GetWidgetChecked<Gtk::ComboBoxText>(
      builder, "cmb_parent_task");

  edt_task_name_->signal_changed().connect(
      sigc::mem_fun(*this, &EditTaskDialog::OnTaskNameChange));
}

void EditTaskDialog::on_response(int response_id) {
  if (response_id == Gtk::RESPONSE_OK && task_) {
    task_->set_name(GetTrimmedEditText());
    task_->set_archived(chk_archived_->get_active());

    const std::string str_task_id = cmb_parent_task_->get_active_id();
    if (str_task_id == kNoneTaskId) {
      task_->set_parent_task_id(std::nullopt);
    } else {
      VERIFY(!str_task_id.empty());
      Task::Id task_id = 0;
      auto [ptr, ec] =  std::from_chars(
          str_task_id.data(),
          str_task_id.data() + str_task_id.size(),
          task_id);
      VERIFY(ec == std::errc());
      VERIFY(ptr = str_task_id.data() + str_task_id.size());
      task_->set_parent_task_id(task_id);
    }
  }
}

void EditTaskDialog::on_show() {
  Gtk::Dialog::on_show();
  const std::vector<Task> child_tasks = LoadChildTasks();
  InitializeParentTaskCombo(child_tasks);
  if (task_) {
    edt_task_name_->set_text(task_->name());
    chk_archived_->set_active(task_->is_archived());
    const bool has_not_arcived_childern = (child_tasks.end() != std::find_if(
        child_tasks.begin(),
        child_tasks.end(),
        [](const Task& t) { return !t.is_archived(); }));
    if (has_not_arcived_childern) {
      chk_archived_->set_sensitive(false);
    } else {
      chk_archived_->set_sensitive(true);
    }
  }
  OnTaskNameChange();
  edt_task_name_->grab_focus();
}

std::vector<Task> EditTaskDialog::LoadChildTasks() noexcept {
  if (task_ && task_->id()) {
    auto maybe_child_tasks = Task::LoadChildTasks(
        &app_state_->db_for_read_only(),
        *task_);
    // TODO(vchigrin): Better error handling.
    VERIFY(maybe_child_tasks);
    return maybe_child_tasks.value();
  } else {
    return std::vector<Task>();
  }
}

void EditTaskDialog::InitializeParentTaskCombo(
    const std::vector<Task>& child_tasks) noexcept {
  cmb_parent_task_->remove_all();
  // TODO(vchigrin): Localization
  cmb_parent_task_->append("<NONE>", kNoneTaskId);
  if (!child_tasks.empty()) {
    // At present we support only one level in tasks hierarchy. Primly
    // due to difficulties in select task UI.
    const bool task_found = cmb_parent_task_->set_active_id(kNoneTaskId);
    VERIFY(task_found);
    cmb_parent_task_->set_sensitive(false);
    return;
  }
  cmb_parent_task_->set_sensitive(true);
  auto maybe_tasks = Task::LoadTopLevel(&app_state_->db_for_read_only());
  // TODO(vchigrin): Better error handling.
  VERIFY(maybe_tasks);
  for (const Task& t : maybe_tasks.value()) {
    VERIFY(t.id());
    const std::string id = std::to_string(*t.id());
    cmb_parent_task_->append(id, t.name());
  }
  if (task_) {
    const std::optional<Task::Id> parent_task_id = task_->parent_task_id();
    bool task_found = false;
    if (parent_task_id) {
      task_found = cmb_parent_task_->set_active_id(
          std::to_string(*parent_task_id));
    } else {
      task_found = cmb_parent_task_->set_active_id(kNoneTaskId);
    }
    VERIFY(task_found);
  }
}

std::string EditTaskDialog::GetTrimmedEditText() noexcept {
  std::string text = edt_task_name_->get_text();
  return boost::algorithm::trim_copy(text);
}

void EditTaskDialog::OnTaskNameChange() noexcept {
  const std::string text = GetTrimmedEditText();
  btn_ok_->set_sensitive(!text.empty());
}

}  // namespace m_time_tracker
