// Copyright 2021 The "TimeKeeper" project authors. All rights reserved.
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

  static std::optional<typename ObjectType::Id> GetObjectIdForRow(
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
  virtual bool FirstObjectShouldPrecedeSecond(
      const ObjectType& first, const ObjectType& second) const noexcept = 0;

  void SetContent(std::vector<ObjectType> objects) noexcept;


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
  // Item in |object_id_to_item_info_|, describing modified element,
  // is not touched. All items with greater positions are updated.
  void RemoveUpdatingIndices(guint position) noexcept;
  guint ComputePositionForItem(const ObjectType& t) const noexcept;

  Gtk::Widget* CreateWidget(const Glib::RefPtr<Glib::Object> obj) noexcept {
    // Caller responsible on releasing reference.
    obj->reference();
    return static_cast<Gtk::Widget*>(obj.get());
  }

  static const Glib::Quark object_id_quark_;

  struct ItemInfo {
    ItemInfo(guint idx, const ObjectType& obj) noexcept
        : item_index(idx),
          object(obj) {}

    guint item_index;
    ObjectType object;
  };

  std::unordered_map<typename ObjectType::Id, ItemInfo> object_id_to_item_info_;
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
void ListModelBase<ObjectType>::SetContent(
    std::vector<ObjectType> objects) noexcept {
  object_id_to_item_info_.clear();
  std::vector<Glib::RefPtr<Gtk::Widget>> items;
  items.reserve(objects.size());
  std::sort(
      objects.begin(),
      objects.end(),
      [this](const ObjectType& first, const ObjectType& second) {
        return FirstObjectShouldPrecedeSecond(first, second);
      });
  for (const ObjectType& t : objects) {
    VERIFY(t.id());
    auto control = DoCreateRowFromObject(t);
    VERIFY(control);
    items.push_back(std::move(control));
    VERIFY(object_id_to_item_info_.insert(
        {*t.id(), ItemInfo(static_cast<guint>(items.size() - 1), t)}).second);
  }
  const guint old_size = get_n_items();
  splice(0U, /* n_removals */ old_size, items);
}

template<typename ObjectType>
void ListModelBase<ObjectType>::ExistingObjectChanged(
    const ObjectType& t) noexcept {
  VERIFY(t.id());
  auto it = object_id_to_item_info_.find(*t.id());
  if (it != object_id_to_item_info_.end()) {
    RemoveUpdatingIndices(it->second.item_index);
    object_id_to_item_info_.erase(it);
  }
  const guint item_position = ComputePositionForItem(t);

  auto new_control = DoCreateRowFromObject(t);
  if (new_control) {
    InsertUpdatindIndeces(item_position, new_control);
    object_id_to_item_info_.insert_or_assign(
        *t.id(), ItemInfo(item_position, t));
  }
}

template<typename ObjectType>
guint ListModelBase<ObjectType>::ComputePositionForItem(
    const ObjectType& t) const noexcept {
  guint item_position = 0;
  for (const auto& [id, info] : object_id_to_item_info_) {
    if (FirstObjectShouldPrecedeSecond(info.object, t)) {
      ++item_position;
    }
  }
  return item_position;
}

template<typename ObjectType>
void ListModelBase<ObjectType>::AfterObjectAdded(
    const ObjectType& t) noexcept {
  VERIFY(t.id());
  Glib::RefPtr<Gtk::Widget> new_control = DoCreateRowFromObject(t);
  if (!new_control) {
    return;
  }
  const guint new_item_index = ComputePositionForItem(t);
  InsertUpdatindIndeces(new_item_index, new_control);
  const auto insert_result = object_id_to_item_info_.insert(
      {*t.id(),
       ItemInfo(new_item_index, t)});
  VERIFY(insert_result.second);
}

template<typename ObjectType>
void ListModelBase<ObjectType>::BeforeObjectDeleted(
    const ObjectType& t) noexcept {
  VERIFY(t.id());
  auto it = object_id_to_item_info_.find(*t.id());
  if (it != object_id_to_item_info_.end()) {
    RemoveUpdatingIndices(it->second.item_index);
    object_id_to_item_info_.erase(it);
  }
}

template<typename ObjectType>
void ListModelBase<ObjectType>::InsertUpdatindIndeces(
    guint position, const Glib::RefPtr<Gtk::Widget>& control) noexcept {
  insert(position, control);
  for (auto& [object_id, object_info] : object_id_to_item_info_) {
    if (object_info.item_index >= position) {
      ++object_info.item_index;
    }
  }
}

template<typename ObjectType>
void ListModelBase<ObjectType>::RemoveUpdatingIndices(guint position) noexcept {
  remove(position);
  for (auto& [object_id, object_info] : object_id_to_item_info_) {
    if (object_info.item_index > position) {
      --object_info.item_index;
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
