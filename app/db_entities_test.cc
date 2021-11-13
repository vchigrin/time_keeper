// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include <optional>
#include <unordered_set>

#include "app/database.h"
#include "app/task.h"
#include "app/verify.h"

using m_time_tracker::Database;
using m_time_tracker::Task;
namespace outcome = m_time_tracker::outcome;

static void AddTaskIdsToSet(
    const std::vector<Task>& tasks,
    std::unordered_set<int64_t>* task_ids) noexcept {
  for (const Task& t : tasks) {
    VERIFY(t.id());
    VERIFY(task_ids->insert(*t.id()).second);
  }
}

class DbEntitiesTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto maybe_db = Database::Open(":memory:");
    if (!maybe_db) {
      std::cerr << "Failed open DB; Error " << maybe_db.error() << std::endl;
      VERIFY(false);
    }
    db_.emplace(std::move(maybe_db.value()));
  }

 protected:
  Database* db() noexcept {
    VERIFY(db_);
    return &*db_;
  }

 private:
  std::optional<Database> db_;
};

TEST_F(DbEntitiesTest, TaskSave) {
  Task new_task("test_task");
  ASSERT_FALSE(new_task.id());  // New task has no id.
  std::error_code save_error = new_task.Save(db());
  ASSERT_FALSE(save_error);
  ASSERT_TRUE(new_task.id());  // Save() fills id.

  {
    outcome::std_result<std::vector<Task>> maybe_all_tasks =
        Task::LoadAll(db());
    ASSERT_TRUE(maybe_all_tasks);
    const std::vector<Task>& all_tasks = maybe_all_tasks.value();
    EXPECT_EQ(all_tasks.size(), 1u);
    EXPECT_EQ(all_tasks[0].id(), new_task.id());
    EXPECT_EQ(all_tasks[0].name(), "test_task");
    EXPECT_EQ(all_tasks[0].is_archived(), false);
  }

  // Now task will be updated.
  const int64_t task_id = *new_task.id();
  new_task.set_name("another name");
  new_task.set_archived(true);
  save_error = new_task.Save(db());
  ASSERT_FALSE(save_error);

  EXPECT_EQ(new_task.id(), task_id);  // Update should not change Task id.

  // Test after update still only one task present.
  {
    outcome::std_result<std::vector<Task>> maybe_all_tasks =
        Task::LoadAll(db());
    ASSERT_TRUE(maybe_all_tasks);
    const std::vector<Task>& all_tasks = maybe_all_tasks.value();
    EXPECT_EQ(all_tasks.size(), 1u);
    EXPECT_EQ(all_tasks[0].id(), task_id);
    EXPECT_EQ(all_tasks[0].name(), "another name");
    EXPECT_EQ(all_tasks[0].is_archived(), true);
  }
}

TEST_F(DbEntitiesTest, TaskLoad) {
  // Parent tasks
  Task foo("foo");
  Task bar("bar");

  std::error_code save_error = foo.Save(db());
  ASSERT_FALSE(save_error);
  save_error = bar.Save(db());
  ASSERT_FALSE(save_error);

  std::vector<Task> children_of_foo, children_of_bar;

  children_of_foo.emplace_back("child1_of_foo");
  children_of_foo.emplace_back("child2_of_foo");
  children_of_foo.emplace_back("child3_of_foo");

  children_of_bar.emplace_back("child1_of_bar");
  children_of_bar.emplace_back("child2_of_bar");

  for (auto& t : children_of_foo) {
    t.SetParentTask(foo);
    save_error = t.Save(db());
    ASSERT_FALSE(save_error);
  }

  for (auto& t : children_of_bar) {
    t.SetParentTask(bar);
    save_error = t.Save(db());
    ASSERT_FALSE(save_error);
  }

  {
    // Test LoadAll
    outcome::std_result<std::vector<Task>> maybe_tasks =
        Task::LoadAll(db());
    ASSERT_TRUE(maybe_tasks);
    const std::vector<Task>& tasks = maybe_tasks.value();
    EXPECT_EQ(
        tasks.size(),
        children_of_foo.size() + children_of_bar.size() + 2u);
    std::unordered_set<int64_t> expected_task_ids {*foo.id(), *bar.id() };
    AddTaskIdsToSet(children_of_foo, &expected_task_ids);
    AddTaskIdsToSet(children_of_bar, &expected_task_ids);

    std::unordered_set<int64_t> loaded_task_ids;
    AddTaskIdsToSet(tasks, &loaded_task_ids);
    EXPECT_EQ(loaded_task_ids, expected_task_ids);
  }

  {
    // Test LoadTopLevel
    outcome::std_result<std::vector<Task>> maybe_tasks =
        Task::LoadTopLevel(db());
    ASSERT_TRUE(maybe_tasks);
    const std::vector<Task>& tasks = maybe_tasks.value();
    EXPECT_EQ(tasks.size(), 2u);
    std::unordered_set<int64_t> expected_task_ids {*foo.id(), *bar.id() };

    std::unordered_set<int64_t> loaded_task_ids;
    AddTaskIdsToSet(tasks, &loaded_task_ids);
    EXPECT_EQ(loaded_task_ids, expected_task_ids);
  }

  {
    // Test LoadChildTasks
    outcome::std_result<std::vector<Task>> maybe_tasks =
        Task::LoadChildTasks(db(), foo);
    ASSERT_TRUE(maybe_tasks);
    const std::vector<Task>& tasks = maybe_tasks.value();
    EXPECT_EQ(tasks.size(), children_of_foo.size());
    std::unordered_set<int64_t> expected_task_ids;
    AddTaskIdsToSet(children_of_foo, &expected_task_ids);

    std::unordered_set<int64_t> loaded_task_ids;
    AddTaskIdsToSet(tasks, &loaded_task_ids);
    EXPECT_EQ(loaded_task_ids, expected_task_ids);
  }

  // Re-assign some "foo" children to "bar" and test again.
  for (size_t i = 1; i < children_of_foo.size(); ++i) {
    children_of_foo[i].SetParentTask(bar);
    children_of_foo[i].Save(db());
  }

  {
    // Test LoadChildTasks
    outcome::std_result<std::vector<Task>> maybe_tasks =
        Task::LoadChildTasks(db(), foo);
    ASSERT_TRUE(maybe_tasks);
    const std::vector<Task>& tasks = maybe_tasks.value();
    EXPECT_EQ(tasks.size(), 1u);
    std::unordered_set<int64_t> expected_task_ids {*children_of_foo[0].id()};

    std::unordered_set<int64_t> loaded_task_ids;
    AddTaskIdsToSet(tasks, &loaded_task_ids);
    EXPECT_EQ(loaded_task_ids, expected_task_ids);
  }
}
