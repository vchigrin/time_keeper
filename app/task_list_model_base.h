// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <sigc++/sigc++.h>
#include <unordered_map>
#include <utility>
#include <vector>

#include "app/task.h"
#include "app/ui_helpers.h"

namespace m_time_tracker {

class DbWrapper;

class TaskListModelBase: public Gio::ListStore<Gtk::Widget> {
 public:
  virtual ~TaskListModelBase();

  sigc::slot<Gtk::Widget*(const Glib::RefPtr<Glib::Object>)>
      slot_create_widget() noexcept {
    return sigc::mem_fun(
        *this,
        &TaskListModelBase::CreateWidget);
  }

  template<typename ChildClass, typename... Args>
  static Glib::RefPtr<ChildClass> create(Args&&... args) noexcept {
    ChildClass* result = new ChildClass(std::forward<Args>(args)...);
    // ref counter is 0 by default, so explicity add refernce here.
    result->reference();
    return Glib::RefPtr<ChildClass>(result);
  }

 protected:
  explicit TaskListModelBase(DbWrapper* db_wrapper) noexcept;

  virtual Glib::RefPtr<Gtk::Widget> CreateRowFromTask(
      const Task& t) noexcept = 0;

  void Initialize(const std::vector<Task>& tasks) noexcept;

 private:
  void ExistingTaskChanged(const Task& t) noexcept;
  void AfterTaskAdded(const Task& t) noexcept;
  void BeforeTaskDeleted(const Task& t) noexcept;
  Gtk::Widget* CreateWidget(const Glib::RefPtr<Glib::Object> obj) noexcept {
    // Caller responsible on releasing reference.
    obj->reference();
    return static_cast<Gtk::Widget*>(obj.get());
  }

  std::unordered_map<int64_t, guint> task_id_to_item_index_;

  std::vector<sigc::connection> all_connections_;
};

}  // namespace m_time_tracker
