// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include <iostream>

#include "app/database.h"

int main(int, char*[]) {
  auto maybe_db = m_time_tracker::Database::Open("/tmp/test.db");
  if (!maybe_db) {
    std::cerr << "Failed open DB; Error " << maybe_db.error() << std::endl;
    return 1;
  }
  std::cout << "Open OK\n";
  m_time_tracker::Database& db = maybe_db.value();
  auto maybe_result = db.Select("SELECT id, name, parent_task_id FROM Tasks");
  if (!maybe_result) {
    std::cerr << "Failed execute Select; Error "
              << maybe_result.error() << std::endl;
    return 1;
  }
  m_time_tracker::SelectRows& rows = maybe_result.value();
  auto Format = [](const std::optional<auto>& data) -> std::string {
    if (data == std::nullopt) {
      return "<NULL>";
    } else {
      if constexpr (
            std::is_same_v<std::decay_t<decltype(*data)>, std::string>) {
        return *data;
      } else {
        return std::to_string(*data);
      }
    }
  };
  while (true) {
    auto err = rows.NextRow();
    if (err) {
      std::cerr << "Next row returns " << err.message() << std::endl;
      break;
    }
    const auto id = rows.IntColumn(0);
    const auto name = rows.StringColumn(1);
    const auto parent_id = rows.IntColumn(2);
    std::cout << "Row: Id=" << Format(id)
              << "; Name=" << Format(name)
              << "; Parent Id=" << Format(parent_id) << std::endl;
  }

  return 0;
}
