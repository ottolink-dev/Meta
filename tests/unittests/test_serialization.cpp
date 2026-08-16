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
