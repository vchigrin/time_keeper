// Copyright 2021 The "MobileTimeTracker" project authors. All rights reserved.
// Use of this source code is governed by a GPLv3 license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include <optional>
#include <unordered_set>

#include "app/activity.h"
#include "app/database.h"
#include "app/task.h"
#include "app/verify.h"

using m_time_tracker::Activity;
using m_time_tracker::Database;
using m_time_tracker::Task;
namespace outcome = m_time_tracker::outcome;

template<typename Type>
static void AddIdsToSet(
    const std::vector<Type>& objects,
    std::unordered_set<int64_t>* object_ids) noexcept {
  for (const Type& t : objects) {
    VERIFY(t.id());
    VERIFY(object_ids->insert(*t.id()).second);
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
    auto create_outcome = Task::EnsureTableCreated(db());
    VERIFY(create_outcome);
    create_outcome = Activity::EnsureTableCreated(db());
    VERIFY(create_outcome);
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
  outcome::std_result<void> save_outcome = new_task.Save(db());
  ASSERT_TRUE(save_outcome);
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
  save_outcome = new_task.Save(db());
  ASSERT_TRUE(save_outcome);

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

  outcome::std_result<void> save_outcome = foo.Save(db());
  ASSERT_TRUE(save_outcome);
  save_outcome = bar.Save(db());
  ASSERT_TRUE(save_outcome);

  std::vector<Task> children_of_foo, children_of_bar;

  children_of_foo.emplace_back("child1_of_foo");
  children_of_foo.emplace_back("child2_of_foo");
  children_of_foo.emplace_back("child3_of_foo");

  children_of_bar.emplace_back("child1_of_bar");
  children_of_bar.emplace_back("child2_of_bar");

  for (auto& t : children_of_foo) {
    t.SetParentTask(foo);
    save_outcome = t.Save(db());
    ASSERT_TRUE(save_outcome);
  }

  for (auto& t : children_of_bar) {
    t.SetParentTask(bar);
    save_outcome = t.Save(db());
    ASSERT_TRUE(save_outcome);
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
    AddIdsToSet(children_of_foo, &expected_task_ids);
    AddIdsToSet(children_of_bar, &expected_task_ids);

    std::unordered_set<int64_t> loaded_task_ids;
    AddIdsToSet(tasks, &loaded_task_ids);
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
    AddIdsToSet(tasks, &loaded_task_ids);
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
    AddIdsToSet(children_of_foo, &expected_task_ids);

    std::unordered_set<int64_t> loaded_task_ids;
    AddIdsToSet(tasks, &loaded_task_ids);
    EXPECT_EQ(loaded_task_ids, expected_task_ids);
  }

  // Re-assign some "foo" children to "bar" and test again.
  for (size_t i = 1; i < children_of_foo.size(); ++i) {
    children_of_foo[i].SetParentTask(bar);
    ASSERT_TRUE(children_of_foo[i].Save(db()));
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
    AddIdsToSet(tasks, &loaded_task_ids);
    EXPECT_EQ(loaded_task_ids, expected_task_ids);
  }
}

TEST_F(DbEntitiesTest, ActivitySave) {
  Task new_task("test_task");
  outcome::std_result<void> save_outcome = new_task.Save(db());
  ASSERT_TRUE(save_outcome);
  const Activity::TimePoint start_time =
      std::chrono::floor<std::chrono::seconds>(
          std::chrono::system_clock::now());
  Activity new_activity(new_task, start_time);
  ASSERT_FALSE(new_activity.id());  // New activity has no id.
  save_outcome = new_activity.Save(db());
  ASSERT_TRUE(save_outcome);
  ASSERT_TRUE(new_activity.id());  // Save() fills id.

  {
    outcome::std_result<std::vector<Activity>> maybe_all_activities =
        Activity::LoadAll(db());
    ASSERT_TRUE(maybe_all_activities);
    const std::vector<Activity>& all_activities = maybe_all_activities.value();
    EXPECT_EQ(all_activities.size(), 1u);
    EXPECT_EQ(all_activities[0].id(), new_activity.id());
    EXPECT_EQ(all_activities[0].task_id(), new_task.id());
    EXPECT_EQ(all_activities[0].start_time(), start_time);
    EXPECT_EQ(all_activities[0].end_time(), std::nullopt);
  }

  // Now activity will be updated.
  const int64_t activity_id = *new_activity.id();
  const Activity::TimePoint new_start = start_time + std::chrono::hours(1);
  const Activity::TimePoint new_end = start_time + std::chrono::hours(2);
  new_activity.SetInterval(new_start, new_end);
  save_outcome = new_activity.Save(db());
  ASSERT_TRUE(save_outcome);

  EXPECT_EQ(new_activity.id(), activity_id);  // Update should not change id.

  // Test after update still only one activity present.
  {
    outcome::std_result<std::vector<Activity>> maybe_all_activities =
        Activity::LoadAll(db());
    ASSERT_TRUE(maybe_all_activities);
    const std::vector<Activity>& all_activities = maybe_all_activities.value();
    EXPECT_EQ(all_activities.size(), 1u);
    EXPECT_EQ(all_activities[0].id(), new_activity.id());
    EXPECT_EQ(all_activities[0].task_id(), new_task.id());
    EXPECT_EQ(all_activities[0].start_time(), new_start);
    EXPECT_EQ(all_activities[0].end_time(), new_end);
  }
}

TEST_F(DbEntitiesTest, ActivityLoad) {
  Task foo("foo");
  Task bar("bar");

  outcome::std_result<void> save_outcome = foo.Save(db());
  ASSERT_TRUE(save_outcome);
  save_outcome = bar.Save(db());
  ASSERT_TRUE(save_outcome);

  const Activity::TimePoint start_time =
      std::chrono::floor<std::chrono::seconds>(
          std::chrono::system_clock::now());

  std::vector<Activity> children_of_foo, children_of_bar;

  children_of_foo.emplace_back(foo, start_time);
  children_of_foo.emplace_back(foo, start_time + std::chrono::minutes(5));
  children_of_foo.emplace_back(foo, start_time + std::chrono::minutes(15));

  children_of_bar.emplace_back(bar, start_time);
  children_of_bar.emplace_back(bar, start_time + std::chrono::minutes(5));

  for (auto& a : children_of_foo) {
    save_outcome = a.Save(db());
    ASSERT_TRUE(save_outcome);
  }

  for (auto& a : children_of_bar) {
    save_outcome = a.Save(db());
    ASSERT_TRUE(save_outcome);
  }

  {
    // Test LoadAll
    outcome::std_result<std::vector<Activity>> maybe_activities =
        Activity::LoadAll(db());
    ASSERT_TRUE(maybe_activities);
    const std::vector<Activity>& activities = maybe_activities.value();
    EXPECT_EQ(
        activities.size(),
        children_of_foo.size() + children_of_bar.size());
    std::unordered_set<int64_t> expected_ids;
    AddIdsToSet(children_of_foo, &expected_ids);
    AddIdsToSet(children_of_bar, &expected_ids);

    std::unordered_set<int64_t> loaded_ids;
    AddIdsToSet(activities, &loaded_ids);
    EXPECT_EQ(loaded_ids, expected_ids);
  }

  {
    // Test LoadAfter
    outcome::std_result<std::vector<Activity>> maybe_activities =
        Activity::LoadAfter(db(), start_time + std::chrono::minutes(5));
    ASSERT_TRUE(maybe_activities);
    const std::vector<Activity>& activities = maybe_activities.value();
    EXPECT_EQ(activities.size(), 3u);
    std::unordered_set<int64_t> expected_ids {
        *children_of_foo[1].id(),
        *children_of_foo[2].id(),
        *children_of_bar[1].id(),
    };

    std::unordered_set<int64_t> loaded_ids;
    AddIdsToSet(activities, &loaded_ids);
    EXPECT_EQ(loaded_ids, expected_ids);
  }
}
