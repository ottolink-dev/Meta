/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta.hpp"
#include <gtest/gtest.h>

TEST(SerializationTest, FullModeRoundTrip)
{
  meta::AttributeContainer src;
  auto                    *a = src.add("param", 42);
  a->metadata().add("description", std::string("A test parameter"));
  a->state().add("expanded", true);

  nlohmann::json j = src.json_to(meta::SerializationMode::full);

  EXPECT_TRUE(j.contains("param"));
  EXPECT_EQ(j["param"]["type"], "int");
  EXPECT_EQ(j["param"]["value"], 42);
  EXPECT_TRUE(j["param"].contains("metadata"));
  EXPECT_EQ(j["param"]["metadata"]["description"]["value"], "A test parameter");
  EXPECT_TRUE(j["param"].contains("state"));
  EXPECT_EQ(j["param"]["state"]["expanded"]["value"], true);

  meta::AttributeContainer dst;
  dst.json_from(j, meta::SerializationMode::full);

  ASSERT_TRUE(dst.contains("param"));
  EXPECT_EQ(dst.value<int>("param"), 42);
  auto *dst_attr = dst.find("param");
  EXPECT_EQ(dst_attr->metadata().value<std::string>("description"),
            "A test parameter");
  EXPECT_EQ(dst_attr->state().value<bool>("expanded"), true);
}

TEST(SerializationTest, StateModeEmitsOnlyValueAndState)
{
  meta::AttributeContainer src;
  auto                    *a = src.add("param", 42);
  a->metadata().add("description", std::string("A test parameter"));
  a->state().add("expanded", true);

  nlohmann::json j = src.json_to(meta::SerializationMode::state);

  ASSERT_TRUE(j.contains("param"));
  EXPECT_FALSE(j["param"].contains("type"));
  EXPECT_FALSE(j["param"].contains("metadata"));
  EXPECT_TRUE(j["param"].contains("value"));
  EXPECT_EQ(j["param"]["value"], 42);
  EXPECT_TRUE(j["param"].contains("state"));
  EXPECT_EQ(j["param"]["state"]["expanded"]["value"], true);
}

TEST(SerializationTest, StateModeIgnoresMetadataAndSkipsUndeclared)
{
  // JSON simulating save file with both declared and undeclared attributes,
  // plus altered metadata
  nlohmann::json save_json = {
      {"declared_param",
       {{"type", "int"},
        {"value", 99},
        {"metadata",
         {{"description",
           {{"type", "std::string"},
            {"value", "Overridden metadata from JSON"}}}}},
        {"state", {{"active", {{"type", "bool"}, {"value", true}}}}}}},
      {"undeclared_param", {{"type", "float"}, {"value", 3.14f}}}};

  meta::AttributeContainer node_container;
  auto                    *declared = node_container.add("declared_param", 10);
  declared->metadata().add("description",
                           std::string("Original static description"));

  node_container.json_from(save_json, meta::SerializationMode::state);

  // 1. Declared attribute value was updated
  EXPECT_EQ(node_container.value<int>("declared_param"), 99);

  // 2. State was restored via factory
  EXPECT_EQ(declared->state().value<bool>("active"), true);

  // 3. Static metadata was NOT overwritten
  EXPECT_EQ(declared->metadata().value<std::string>("description"),
            "Original static description");

  // 4. Undeclared attribute was skipped
  EXPECT_FALSE(node_container.contains("undeclared_param"));
}

TEST(SerializationTest, StateModeReconstructsDynamicStateAttributes)
{
  meta::AttributeContainer container;
  auto                    *attr = container.add("vec", 5);

  nlohmann::json state_json = {
      {"vec",
       {{"value", 5},
        {"state",
         {{"ui.state", {{"type", "bool"}, {"value", true}}},
          {"custom_tag", {{"type", "std::string"}, {"value", "dynamic"}}}}}}}};

  container.json_from(state_json, meta::SerializationMode::state);

  ASSERT_TRUE(attr->state().contains("ui.state"));
  EXPECT_EQ(attr->state().value<bool>("ui.state"), true);

  ASSERT_TRUE(attr->state().contains("custom_tag"));
  EXPECT_EQ(attr->state().value<std::string>("custom_tag"), "dynamic");
}

TEST(SerializationTest, ContainerStateModeRoundTrip)
{
  meta::AttributeContainer src;
  src.add("param", 42);
  src.state().add("section.is_expanded", false);

  nlohmann::json j = src.json_to(meta::SerializationMode::state);

  EXPECT_TRUE(j.contains("param"));
  EXPECT_TRUE(j.contains("state"));
  EXPECT_TRUE(j["state"].contains("section.is_expanded"));
  EXPECT_EQ(j["state"]["section.is_expanded"]["value"], false);

  meta::AttributeContainer dst;
  dst.add("param", 0);
  dst.json_from(j, meta::SerializationMode::state);

  EXPECT_EQ(dst.value<int>("param"), 42);
  ASSERT_TRUE(dst.state().contains("section.is_expanded"));
  EXPECT_EQ(dst.state().value<bool>("section.is_expanded"), false);
}

TEST(SerializationTest, ContainerGroupFullModeRoundTrip)
{
  meta::ContainerGroup src;
  auto                &v1 = src.add("view1");
  auto                *a1 = v1.add("param1", 42);
  a1->metadata().add("desc", std::string("description 1"));
  a1->state().add("active", true);

  auto &v2 = src.add("view2");
  v2.add("param2", 3.14f);

  src.set_current("view2");

  nlohmann::json j = src.json_to(meta::SerializationMode::full);

  EXPECT_TRUE(j.contains("current"));
  EXPECT_EQ(j["current"], "view2");
  EXPECT_TRUE(j.contains("containers"));
  EXPECT_TRUE(j["containers"].contains("view1"));
  EXPECT_TRUE(j["containers"].contains("view2"));

  meta::ContainerGroup dst;
  dst.json_from(j, meta::SerializationMode::full);

  EXPECT_EQ(dst.size(), 2);
  ASSERT_TRUE(dst.contains("view1"));
  ASSERT_TRUE(dst.contains("view2"));

  ASSERT_TRUE(dst.current_container_name().has_value());
  EXPECT_EQ(dst.current_container_name().value(), "view2");

  auto *dst_v1 = dst.find("view1");
  ASSERT_NE(dst_v1, nullptr);
  EXPECT_EQ(dst_v1->value<int>("param1"), 42);
  auto *dst_a1 = dst_v1->find("param1");
  ASSERT_NE(dst_a1, nullptr);
  EXPECT_EQ(dst_a1->metadata().value<std::string>("desc"), "description 1");
  EXPECT_EQ(dst_a1->state().value<bool>("active"), true);

  auto *dst_v2 = dst.find("view2");
  ASSERT_NE(dst_v2, nullptr);
  EXPECT_FLOAT_EQ(dst_v2->value<float>("param2"), 3.14f);

  const auto &order = dst.insertion_order();
  ASSERT_EQ(order.size(), 2);
  EXPECT_EQ(order[0], "view1");
  EXPECT_EQ(order[1], "view2");
}

TEST(SerializationTest, ContainerGroupStateMode)
{
  meta::ContainerGroup src;
  auto                &v1 = src.add("view1");
  auto                *a1 = v1.add("param1", 100);
  a1->metadata().add("desc", std::string("static desc"));
  a1->state().add("active", true);

  nlohmann::json j = src.json_to(meta::SerializationMode::state);

  meta::ContainerGroup dst;
  auto                &dst_v1 = dst.add("view1");
  auto                *dst_a1 = dst_v1.add("param1", 10);
  dst_a1->metadata().add("desc", std::string("dst static desc"));

  dst.json_from(j, meta::SerializationMode::state);

  EXPECT_EQ(dst_v1.value<int>("param1"), 100);
  EXPECT_EQ(dst_a1->metadata().value<std::string>("desc"), "dst static desc");
  EXPECT_EQ(dst_a1->state().value<bool>("active"), true);

  // Undeclared containers are skipped in state mode
  nlohmann::json save_json = {
      {"current", "undeclared_view"},
      {"containers", {{"undeclared_view", {{"paramX", {{"value", 999}}}}}}}};

  dst.json_from(save_json, meta::SerializationMode::state);
  EXPECT_FALSE(dst.contains("undeclared_view"));
}

TEST(SerializationTest, ContainerGroupClear)
{
  meta::ContainerGroup group;
  group.add("c1");
  group.add("c2");
  group.set_current("c2");

  EXPECT_EQ(group.size(), 2);
  EXPECT_TRUE(group.current_container_name().has_value());

  group.clear();

  EXPECT_EQ(group.size(), 0);
  EXPECT_FALSE(group.current_container_name().has_value());
  EXPECT_TRUE(group.insertion_order().empty());
}
