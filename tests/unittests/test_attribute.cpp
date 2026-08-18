/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta.hpp"
#include <gtest/gtest.h>

TEST(AttributeTest, ConstructionAndValue)
{
  meta::Attribute<int> attr("my_int", 42);
  EXPECT_EQ(attr.name(), "my_int");
  EXPECT_EQ(attr.value(), 42);
  EXPECT_EQ(attr.type(), std::type_index(typeid(int)));
  EXPECT_EQ(attr.to_string(), "42");

  attr.value() = 100;
  EXPECT_EQ(attr.value(), 100);
}

TEST(AttributeTest, TypeErasedAccess)
{
  meta::Attribute<std::string> attr("my_str", "hello");
  EXPECT_EQ(std::any_cast<std::string>(attr.to_any()), "hello");

  EXPECT_TRUE(attr.set_from_any(std::string("world")));
  EXPECT_EQ(attr.value(), "world");

  EXPECT_FALSE(attr.set_from_any(123)); // wrong type
  EXPECT_EQ(attr.value(), "world");
}

TEST(AttributeTest, EventNotification)
{
  meta::Attribute<int> attr("my_int", 10);
  int                  received = 0;
  auto                 conn = attr.value_changed.subscribe([&received](int val)
                                           { received = val; });

  attr.set_from_any(25);
  EXPECT_EQ(received, 25);
}

TEST(AttributeTest, MetadataAndStateSeparation)
{
  meta::Attribute<float> attr("speed", 1.5f);

  // Static metadata
  attr.metadata().add("label", std::string("Speed (m/s)"));
  attr.metadata().add("min", 0.0f);
  attr.metadata().add("max", 10.0f);

  // Runtime state
  attr.state().add("active", true);
  attr.state().add("collapsed", false);

  EXPECT_EQ(attr.metadata().value<std::string>("label"), "Speed (m/s)");
  EXPECT_FLOAT_EQ(attr.metadata().value<float>("min"), 0.0f);
  EXPECT_FLOAT_EQ(attr.metadata().value<float>("max"), 10.0f);

  EXPECT_EQ(attr.state().value<bool>("active"), true);
  EXPECT_EQ(attr.state().value<bool>("collapsed"), false);
}
