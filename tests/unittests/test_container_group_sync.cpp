/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <gtest/gtest.h>

#include "meta/core/container_group.hpp"

TEST(ContainerGroupSyncTest, DetectSharedAttributes)
{
  meta::ContainerGroup group;

  auto &c1 = group.add("Feature1");
  c1.add("scale", 1.0f);
  c1.add("intensity", 0.5f);
  c1.add("specific1", 10);
  c1.add("tag", std::string("alpha"));

  auto &c2 = group.add("Feature2");
  c2.add("scale", 2.0f);
  c2.add("intensity", 0.8f);
  c2.add("specific2", 20);
  c2.add("tag", std::string("beta"));

  auto &c3 = group.add("Feature3");
  c3.add("scale", 3.0f);
  c3.add("specific3", 30);
  c3.add("tag", 123); // different type (int vs string)

  // scale is in c1, c2, c3 (float) -> shared
  // intensity is in c1, c2 (float) -> shared
  // tag is in c1, c2 (string) -> shared (between c1 and c2)
  // specific1, specific2, specific3 -> not shared

  EXPECT_TRUE(group.is_shared("scale"));
  EXPECT_TRUE(group.is_shared("intensity"));
  EXPECT_TRUE(group.is_shared("tag"));
  EXPECT_FALSE(group.is_shared("specific1"));
  EXPECT_FALSE(group.is_shared("specific2"));
  EXPECT_FALSE(group.is_shared("specific3"));
  EXPECT_FALSE(group.is_shared("nonexistent"));

  auto shared = group.shared_attributes();
  EXPECT_EQ(shared.size(), 3u);
  EXPECT_EQ(shared[0], "scale");
  EXPECT_EQ(shared[1], "intensity");
  EXPECT_EQ(shared[2], "tag");
}

TEST(ContainerGroupSyncTest, BasicSynchronizationAndIndependence)
{
  meta::ContainerGroup group;

  auto &c1 = group.add("Feature1");
  c1.add("scale", 1.0f);
  c1.add("intensity", 0.5f);
  c1.add("offset", 10);

  auto &c2 = group.add("Feature2");
  c2.add("scale", 2.0f);
  c2.add("intensity", 0.8f);
  c2.add("offset", 20);

  // Mark only "scale" as synchronized
  group.set_synchronized("scale", true);
  EXPECT_TRUE(group.is_synchronized("scale"));
  EXPECT_FALSE(group.is_synchronized("intensity"));
  EXPECT_FALSE(group.is_synchronized("offset"));

  // Initial sync should propagate current_ (Feature1) value (1.0f) to Feature2
  EXPECT_FLOAT_EQ(c1.value<float>("scale"), 1.0f);
  EXPECT_FLOAT_EQ(c2.value<float>("scale"), 1.0f);

  // Non-synchronized attributes should remain independent
  EXPECT_FLOAT_EQ(c1.value<float>("intensity"), 0.5f);
  EXPECT_FLOAT_EQ(c2.value<float>("intensity"), 0.8f);
  EXPECT_EQ(c1.value<int>("offset"), 10);
  EXPECT_EQ(c2.value<int>("offset"), 20);

  // Update scale on c1 via set_value -> c2 should update
  c1.find("scale")->try_cast<meta::Attribute<float>>()->set_value(5.0f);
  EXPECT_FLOAT_EQ(c1.value<float>("scale"), 5.0f);
  EXPECT_FLOAT_EQ(c2.value<float>("scale"), 5.0f);

  // Update scale on c2 via set_from_any -> c1 should update
  c2.find("scale")->set_from_any(std::make_any<float>(7.5f));
  EXPECT_FLOAT_EQ(c1.value<float>("scale"), 7.5f);
  EXPECT_FLOAT_EQ(c2.value<float>("scale"), 7.5f);

  // Changing non-synchronized attribute on c1 does not affect c2
  c1.find("intensity")->try_cast<meta::Attribute<float>>()->set_value(0.1f);
  EXPECT_FLOAT_EQ(c1.value<float>("intensity"), 0.1f);
  EXPECT_FLOAT_EQ(c2.value<float>("intensity"), 0.8f);
}

TEST(ContainerGroupSyncTest, EventNotificationOnSync)
{
  meta::ContainerGroup group;

  auto &c1 = group.add("Feature1");
  c1.add("scale", 1.0f);

  auto &c2 = group.add("Feature2");
  c2.add("scale", 2.0f);

  group.set_synchronized("scale", true);

  float c2_notified_value = 0.0f;
  int   c2_notification_count = 0;

  auto *c2_attr = c2.find("scale")->try_cast<meta::Attribute<float>>();
  ASSERT_NE(c2_attr, nullptr);

  auto conn = c2_attr->value_changed.subscribe(
      [&](float val)
      {
        c2_notified_value = val;
        c2_notification_count++;
      });

  // Update c1
  c1.find("scale")->try_cast<meta::Attribute<float>>()->set_value(9.0f);

  EXPECT_FLOAT_EQ(c1.value<float>("scale"), 9.0f);
  EXPECT_FLOAT_EQ(c2.value<float>("scale"), 9.0f);
  EXPECT_EQ(c2_notification_count, 1);
  EXPECT_FLOAT_EQ(c2_notified_value, 9.0f);
}

TEST(ContainerGroupSyncTest, EnableDisableSynchronization)
{
  meta::ContainerGroup group;

  auto &c1 = group.add("Feature1");
  c1.add("scale", 1.0f);

  auto &c2 = group.add("Feature2");
  c2.add("scale", 2.0f);

  group.set_synchronized("scale", true);
  EXPECT_TRUE(group.is_synchronized("scale"));
  EXPECT_FLOAT_EQ(c2.value<float>("scale"), 1.0f);

  // Disable synchronization
  group.set_synchronized("scale", false);
  EXPECT_FALSE(group.is_synchronized("scale"));

  // Updating c1 should not affect c2 anymore
  c1.find("scale")->try_cast<meta::Attribute<float>>()->set_value(10.0f);
  EXPECT_FLOAT_EQ(c1.value<float>("scale"), 10.0f);
  EXPECT_FLOAT_EQ(c2.value<float>("scale"), 1.0f);

  // Updating c2 should not affect c1
  c2.find("scale")->try_cast<meta::Attribute<float>>()->set_value(20.0f);
  EXPECT_FLOAT_EQ(c1.value<float>("scale"), 10.0f);
  EXPECT_FLOAT_EQ(c2.value<float>("scale"), 20.0f);
}

TEST(ContainerGroupSyncTest, SynchronizeAllAndClear)
{
  meta::ContainerGroup group;

  auto &c1 = group.add("Feature1");
  c1.add("a", 1.0f);
  c1.add("b", 2.0f);
  c1.add("c", 3);

  auto &c2 = group.add("Feature2");
  c2.add("a", 10.0f);
  c2.add("b", 20.0f);
  c2.add("c", 30);

  group.synchronize_all(true);
  EXPECT_TRUE(group.is_synchronized("a"));
  EXPECT_TRUE(group.is_synchronized("b"));
  EXPECT_TRUE(group.is_synchronized("c"));

  EXPECT_FLOAT_EQ(c2.value<float>("a"), 1.0f);
  EXPECT_FLOAT_EQ(c2.value<float>("b"), 2.0f);
  EXPECT_EQ(c2.value<int>("c"), 3);

  group.clear_synchronizations();
  EXPECT_FALSE(group.is_synchronized("a"));
  EXPECT_FALSE(group.is_synchronized("b"));
  EXPECT_FALSE(group.is_synchronized("c"));
  EXPECT_TRUE(group.synchronized_attributes().empty());
}

TEST(ContainerGroupSyncTest, DynamicAdditionOfContainersAndAttributes)
{
  meta::ContainerGroup group;

  auto &c1 = group.add("Feature1");
  c1.add("scale", 1.0f);

  auto &c2 = group.add("Feature2");
  c2.add("scale", 2.0f);

  group.set_synchronized("scale", true);
  EXPECT_FLOAT_EQ(c2.value<float>("scale"), 1.0f);

  // Add a new container dynamically
  auto &c3 = group.add("Feature3");
  // Add synchronized attribute to c3
  c3.add("scale", 99.0f);

  // c3 should automatically receive the synchronized value
  EXPECT_FLOAT_EQ(c3.value<float>("scale"), 1.0f);

  // Updating c3 should update c1 and c2
  c3.find("scale")->try_cast<meta::Attribute<float>>()->set_value(42.0f);
  EXPECT_FLOAT_EQ(c1.value<float>("scale"), 42.0f);
  EXPECT_FLOAT_EQ(c2.value<float>("scale"), 42.0f);
  EXPECT_FLOAT_EQ(c3.value<float>("scale"), 42.0f);
}

TEST(ContainerGroupSyncTest, EraseContainerMaintainsSync)
{
  meta::ContainerGroup group;

  auto &c1 = group.add("Feature1");
  c1.add("scale", 1.0f);

  auto &c2 = group.add("Feature2");
  c2.add("scale", 2.0f);

  auto &c3 = group.add("Feature3");
  c3.add("scale", 3.0f);

  group.set_synchronized("scale", true);

  // Erase c2
  EXPECT_TRUE(group.erase("Feature2"));
  EXPECT_EQ(group.size(), 2u);

  // c1 and c3 continue to sync
  c1.find("scale")->try_cast<meta::Attribute<float>>()->set_value(15.0f);
  EXPECT_FLOAT_EQ(c1.value<float>("scale"), 15.0f);
  EXPECT_FLOAT_EQ(c3.value<float>("scale"), 15.0f);
}

TEST(ContainerGroupSyncTest, JsonRoundTripPreservesSync)
{
  meta::ContainerGroup src;

  auto &c1 = src.add("Feature1");
  c1.add("scale", 1.0f);
  c1.add("name", std::string("Default"));

  auto &c2 = src.add("Feature2");
  c2.add("scale", 2.0f);
  c2.add("name", std::string("Custom"));

  src.set_synchronized("scale", true);

  nlohmann::json j = src.json_to(meta::SerializationMode::full);
  EXPECT_TRUE(j.contains("synchronized_attributes"));

  meta::ContainerGroup dst;
  dst.json_from(j, meta::SerializationMode::full);

  EXPECT_TRUE(dst.is_synchronized("scale"));
  EXPECT_FALSE(dst.is_synchronized("name"));

  auto *dst_c1 = dst.find("Feature1");
  auto *dst_c2 = dst.find("Feature2");
  ASSERT_NE(dst_c1, nullptr);
  ASSERT_NE(dst_c2, nullptr);

  EXPECT_FLOAT_EQ(dst_c1->value<float>("scale"), 1.0f);
  EXPECT_FLOAT_EQ(dst_c2->value<float>("scale"), 1.0f);

  // Verify synchronization is active in deserialized group
  dst_c1->find("scale")->try_cast<meta::Attribute<float>>()->set_value(88.0f);
  EXPECT_FLOAT_EQ(dst_c1->value<float>("scale"), 88.0f);
  EXPECT_FLOAT_EQ(dst_c2->value<float>("scale"), 88.0f);
}
