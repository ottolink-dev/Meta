/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta.hpp"
#include <gtest/gtest.h>

TEST(AttributeContainerTest, AddAndLookup)
{
  meta::AttributeContainer container;
  EXPECT_TRUE(container.empty());
  EXPECT_EQ(container.size(), 0);

  container.add("int_val", 10);
  container.add("str_val", std::string("abc"));

  EXPECT_FALSE(container.empty());
  EXPECT_EQ(container.size(), 2);

  EXPECT_TRUE(container.contains("int_val"));
  EXPECT_TRUE(container.contains("str_val"));
  EXPECT_FALSE(container.contains("non_existent"));

  EXPECT_EQ(container.value<int>("int_val"), 10);
  EXPECT_EQ(container.value<std::string>("str_val"), "abc");

  const int *p_val = container.try_value<int>("int_val");
  ASSERT_NE(p_val, nullptr);
  EXPECT_EQ(*p_val, 10);

  const float *p_wrong = container.try_value<float>("int_val");
  EXPECT_EQ(p_wrong, nullptr);
}

TEST(AttributeContainerTest, TryAdd)
{
  meta::AttributeContainer container;
  auto                    *a1 = container.try_add("key", 1);
  EXPECT_EQ(a1->value(), 1);

  auto *a2 = container.try_add("key", 2);
  EXPECT_EQ(a2->value(), 1); // Not overwritten
  EXPECT_EQ(container.size(), 1);
}

TEST(AttributeContainerTest, InsertionOrder)
{
  meta::AttributeContainer container;
  container.add("c", 3);
  container.add("a", 1);
  container.add("b", 2);

  EXPECT_EQ(container.insertion_order(),
            (std::vector<std::string>{"c", "a", "b"}));

  EXPECT_TRUE(container.set_insertion_order({"a", "b", "c"}));
  EXPECT_EQ(container.insertion_order(),
            (std::vector<std::string>{"a", "b", "c"}));

  // Invalid sizes or keys should fail and leave order unchanged
  EXPECT_FALSE(container.set_insertion_order({"a", "b"}));
  EXPECT_EQ(container.insertion_order(),
            (std::vector<std::string>{"a", "b", "c"}));

  EXPECT_FALSE(container.set_insertion_order({"a", "b", "z"}));
  EXPECT_EQ(container.insertion_order(),
            (std::vector<std::string>{"a", "b", "c"}));
}
