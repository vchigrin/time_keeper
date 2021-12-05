// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#pragma once

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wsign-conversion"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <gtkmm.h>
#include <handy.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

#include <memory>
#include <utility>
#include "app/verify.h"

namespace m_time_tracker {

// Wraps top-level widget. Note that C++ object for it is alive till
// Gtk::Builder is alive since it holds reference to it.
template<typename T, typename... Args>
Glib::RefPtr<T> GetWindowDerived(
    const Glib::RefPtr<Gtk::Builder>& builder,
    const Glib::ustring& name,
    Args&&... args) noexcept {
  T* result = nullptr;
  VERIFY(builder);
  builder->get_widget_derived(name, result, std::forward<Args>(args)...);
  VERIFY(result);
  Glib::RefPtr<T> result_ptr(result);
  result_ptr->reference();
  return result_ptr;
}

template<typename T>
T* GetWidgetChecked(
    const Glib::RefPtr<Gtk::Builder>& builder,
    const Glib::ustring& name) noexcept {
  T* result = nullptr;
  VERIFY(builder);
  builder->get_widget(name, result);
  VERIFY(result);
  return result;
}

}  // namespace m_time_tracker
