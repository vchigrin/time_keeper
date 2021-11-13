// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include <iostream>

#include "app/database.h"
#include "app/task.h"


void PrintChildren(
    m_time_tracker::Database* db,
    const m_time_tracker::Task& parent) {
  auto maybe_tasks = m_time_tracker::Task::LoadChildTasks(db, parent);
  if (!maybe_tasks) {
    std::cerr << "Failed execute SelECT; Error "
              << maybe_tasks.error().message() << std::endl;
    return;
  }
  const std::vector<m_time_tracker::Task>& tasks = maybe_tasks.value();
  std::cout << std::boolalpha;
  for (const auto& task : tasks) {
    std::cout << "    ChildTask: Name=" << task.name()
              << "; IsArchived=" << task.is_archived()
              << std::endl;
  }
}

int main(int, char*[]) {
  auto maybe_db = m_time_tracker::Database::Open("/tmp/test.db");
  if (!maybe_db) {
    std::cerr << "Failed open DB; Error " << maybe_db.error() << std::endl;
    return 1;
  }
  std::cout << "Open OK\n";
  m_time_tracker::Database& db = maybe_db.value();
  m_time_tracker::Task new_task("88888");
  std::error_code save_result = new_task.Save(&db);
  if (save_result) {
    std::cerr << "Failed save new task "
              << save_result.message() << std::endl;;
  } else {
    std::cout << "New task saved; Id = " << *new_task.id() << std::endl;
  }
  new_task.set_archived(true);
  save_result = new_task.Save(&db);
  if (save_result) {
    std::cerr << "Failed update new task "
              << save_result.message() << std::endl;;
  } else {
    std::cout << "New task updated; Id = " << *new_task.id() << std::endl;
  }

  auto maybe_tasks = m_time_tracker::Task::LoadTopLevel(&db);
  if (!maybe_tasks) {
    std::cerr << "Failed execute SelECT; Error "
              << maybe_tasks.error().message() << std::endl;
    return 1;
  }
  const std::vector<m_time_tracker::Task>& tasks = maybe_tasks.value();
  std::cout << std::boolalpha;
  for (const auto& task : tasks) {
    std::cout << "Task: Name=" << task.name()
              << "; ID= " << *task.id()
              << "; IsArchived=" << task.is_archived()
              << std::endl;
    PrintChildren(&db, task);
  }

  return 0;
}
