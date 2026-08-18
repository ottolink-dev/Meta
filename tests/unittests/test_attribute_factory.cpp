/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta.hpp"
#include <gtest/gtest.h>

TEST(AttributeFactoryTest, CreateBuiltinTypes)
{
  auto &factory = meta::AttributeFactory::instance();

  auto int_attr = factory.create("int", "test_int");
  ASSERT_NE(int_attr, nullptr);
  EXPECT_EQ(int_attr->name(), "test_int");
  EXPECT_EQ(int_attr->type(), std::type_index(typeid(int)));

  auto str_attr = factory.create("std::string", "test_str");
  ASSERT_NE(str_attr, nullptr);
  EXPECT_EQ(str_attr->name(), "test_str");
  EXPECT_EQ(str_attr->type(), std::type_index(typeid(std::string)));

  auto float_attr = factory.create("float", "test_float");
  ASSERT_NE(float_attr, nullptr);
  EXPECT_EQ(float_attr->name(), "test_float");
  EXPECT_EQ(float_attr->type(), std::type_index(typeid(float)));

  auto unknown_attr = factory.create("unknown_type_name_xyz", "bad");
  EXPECT_EQ(unknown_attr, nullptr);
}
