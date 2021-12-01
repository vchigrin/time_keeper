// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#include <sigc++/sigc++.h>
#include <unordered_map>
#include <utility>
#include <vector>

#include "app/ui_helpers.h"

namespace m_time_tracker {

class AppState;

template<typename ObjectType>
class ListModelBase: public Gio::ListStore<Gtk::Widget> {
 public:
  virtual ~ListModelBase() {
    for (auto& connection : all_connections_) {
      connection.disconnect();
    }
  }

  sigc::slot<Gtk::Widget*(const Glib::RefPtr<Glib::Object>)>
      slot_create_widget() noexcept {
    return sigc::mem_fun(
        *this,
        &ListModelBase<ObjectType>::CreateWidget);
  }

  template<typename ChildClass, typename... Args>
  static Glib::RefPtr<ChildClass> create(Args&&... args) noexcept {
    ChildClass* result = new ChildClass(std::forward<Args>(args)...);
    // ref counter is 0 by default, so explicity add refernce here.
    result->reference();
    return Glib::RefPtr<ChildClass>(result);
  }

  std::optional<typename ObjectType::Id> GetObjectIdForRow(
      Gtk::ListBoxRow* row) noexcept;

 protected:
  explicit ListModelBase(AppState* app_state) noexcept
      : app_state_(app_state) {
    VERIFY(app_state_);
  }

  // May return nullptr if this Object must not be displayed in current
  // list view.
  virtual Glib::RefPtr<Gtk::Widget> CreateRowFromObject(
      const ObjectType& o) noexcept = 0;

  void Initialize(const std::vector<ObjectType>& objects) noexcept;

  void ExistingObjectChanged(const ObjectType& t) noexcept;
  void AfterObjectAdded(const ObjectType& t) noexcept;
  void BeforeObjectDeleted(const ObjectType& t) noexcept;

  // If you need subscribe to some signals, add connections here.
  std::vector<sigc::connection> all_connections_;

  AppState* const app_state_;

 private:
  Glib::RefPtr<Gtk::Widget> DoCreateRowFromObject(
      const ObjectType& o) noexcept;

  void InsertUpdatindIndeces(
      guint position, const Glib::RefPtr<Gtk::Widget>& control) noexcept;
  // Item in |object_id_to_item_index_|, describing modified element,
  // is not touched. All items with greater positions are updated.
  void RemoveUpdatingIndices(guint position) noexcept;

  Gtk::Widget* CreateWidget(const Glib::RefPtr<Glib::Object> obj) noexcept {
    // Caller responsible on releasing reference.
    obj->reference();
    return static_cast<Gtk::Widget*>(obj.get());
  }

  static const Glib::Quark object_id_quark_;

  std::unordered_map<typename ObjectType::Id, guint> object_id_to_item_index_;
};

template<typename ObjectType>
inline const Glib::Quark ListModelBase<ObjectType>::object_id_quark_(
    "object-list-object-id");

template<typename ObjectType>
Glib::RefPtr<Gtk::Widget> ListModelBase<ObjectType>::DoCreateRowFromObject(
    const ObjectType& o) noexcept {
  auto new_control = CreateRowFromObject(o);
  if (!new_control) {
    return new_control;
  }
  auto delete_id = [](gpointer ptr) {
    delete reinterpret_cast<typename ObjectType::Id*>(ptr);
  };
  // We have to copy data in pointer rather then store id as pointer,
  // since Object may allow zero id, that we can not distinguish
  // from control without data.
  new_control->set_data(
      object_id_quark_, new typename ObjectType::Id(*o.id()), delete_id);
  return new_control;
}

template<typename ObjectType>
void ListModelBase<ObjectType>::Initialize(
    const std::vector<ObjectType>& objects) noexcept {
  VERIFY(object_id_to_item_index_.empty());
  std::vector<Glib::RefPtr<Gtk::Widget>> items;
  items.reserve(objects.size());
  for (const ObjectType& t : objects) {
    VERIFY(t.id());
    auto control = DoCreateRowFromObject(t);
    VERIFY(control);
    items.push_back(std::move(control));
    VERIFY(object_id_to_item_index_.insert(
        {*t.id(), items.size() - 1}).second);
  }
  splice(0U, 0U, items);
}

template<typename ObjectType>
void ListModelBase<ObjectType>::ExistingObjectChanged(
    const ObjectType& t) noexcept {
  VERIFY(t.id());
  auto it = object_id_to_item_index_.find(*t.id());
  guint item_position = 0;
  if (it != object_id_to_item_index_.end()) {
    item_position = it->second;
    RemoveUpdatingIndices(item_position);
  } else {
    item_position = get_n_items();
  }

  auto new_control = DoCreateRowFromObject(t);
  if (new_control) {
    InsertUpdatindIndeces(item_position, new_control);
    object_id_to_item_index_[*t.id()] = item_position;
  } else {
    if (it != object_id_to_item_index_.end()) {
      object_id_to_item_index_.erase(it);
    }
  }
}

template<typename ObjectType>
void ListModelBase<ObjectType>::AfterObjectAdded(
    const ObjectType& t) noexcept {
  VERIFY(t.id());
  Glib::RefPtr<Gtk::Widget> new_control = DoCreateRowFromObject(t);
  if (!new_control) {
    return;
  }
  const guint new_item_index = get_n_items();
  const auto insert_result = object_id_to_item_index_.insert(
      {*t.id(), new_item_index});
  VERIFY(insert_result.second);
  append(new_control);
}

template<typename ObjectType>
void ListModelBase<ObjectType>::BeforeObjectDeleted(
    const ObjectType& t) noexcept {
  VERIFY(t.id());
  auto it = object_id_to_item_index_.find(*t.id());
  if (it != object_id_to_item_index_.end()) {
    RemoveUpdatingIndices(it->second);
    object_id_to_item_index_.erase(it);
  }
}

template<typename ObjectType>
void ListModelBase<ObjectType>::InsertUpdatindIndeces(
    guint position, const Glib::RefPtr<Gtk::Widget>& control) noexcept {
  insert(position, control);
  for (auto& [object_id, object_pos] : object_id_to_item_index_) {
    if (object_pos >= position) {
      ++object_pos;
    }
  }
}

template<typename ObjectType>
void ListModelBase<ObjectType>::RemoveUpdatingIndices(guint position) noexcept {
  remove(position);
  for (auto& [object_id, object_pos] : object_id_to_item_index_) {
    if (object_pos > position) {
      --object_pos;
    }
  }
}

template<typename ObjectType>
std::optional<typename ObjectType::Id>
    ListModelBase<ObjectType>::GetObjectIdForRow(
        Gtk::ListBoxRow* row) noexcept {
  const gpointer data = row->get_data(object_id_quark_);
  if (!data) {
    return std::nullopt;
  } else {
    return *static_cast<const typename ObjectType::Id*>(data);
  }
}


}  // namespace m_time_tracker
