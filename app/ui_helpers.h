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
#endif

#include <gtkmm.h>
#include <handy.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

#include <memory>
#include "app/verify.h"

namespace m_time_tracker {

// Wraps top-level widget, that must be deleted by application code, and not
// by Gtk::Builder.
template<typename T>
std::unique_ptr<T> GetWindowDerived(
    const Glib::RefPtr<Gtk::Builder>& builder,
    const Glib::ustring& name) noexcept {
  T* result = nullptr;
  VERIFY(builder);
  builder->get_widget_derived(name, result);
  VERIFY(result);
  return std::unique_ptr<T>(result);
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
