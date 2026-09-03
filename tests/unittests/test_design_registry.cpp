/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <gtest/gtest.h>

#include "meta/core/attribute.hpp"
#include "meta/core/attribute_container.hpp"
#include "meta_common.hpp"
#include "meta_qt/container_widget.hpp"
#include "meta_qt/designs/industrial/industrial.hpp"
#include "meta_qt/designs/stock/stock.hpp"
#include "meta_qt/ui/control.hpp"
#include "meta_qt/ui/design_registry.hpp"
#include "meta_qt/ui/theme.hpp"

namespace
{

// Dummy test control that always accepts
class DummyAcceptControl : public meta::qt::Control<float>
{
public:
  DummyAcceptControl(meta::Attribute<float>     &attr,
                     const meta::qt::RowContext &ctx,
                     QWidget                    *parent = nullptr)
      : meta::qt::Control<float>(ctx, parent), value_(attr.value())
  {
  }

  static bool can_render(const meta::Attribute<float> &) { return true; }

  float get() const override { return value_; }
  void  set(const float &val) override { value_ = val; }

private:
  float value_ = 0.f;
};

// Dummy test control that always rejects
class DummyRejectControl : public meta::qt::Control<float>
{
public:
  DummyRejectControl(meta::Attribute<float>     &attr,
                     const meta::qt::RowContext &ctx,
                     QWidget                    *parent = nullptr)
      : meta::qt::Control<float>(ctx, parent), value_(attr.value())
  {
  }

  static bool can_render(const meta::Attribute<float> &) { return false; }

  float get() const override { return value_; }
  void  set(const float &val) override { value_ = val; }

private:
  float value_ = 0.f;
};

} // namespace

TEST(DesignRegistryTest, RegistrationAndLookup)
{
  // --- Test basic registration and lookup

  auto &registry = meta::qt::DesignRegistry::instance();
  registry.register_control<float, DummyAcceptControl>("test_design",
                                                       "CustomFloat");

  EXPECT_TRUE(registry.has_design("test_design"));
  EXPECT_TRUE(registry.has_control("test_design",
                                   std::type_index(typeid(float)),
                                   "CustomFloat"));
  EXPECT_FALSE(registry.has_control("test_design",
                                    std::type_index(typeid(int)),
                                    "CustomFloat"));
}

TEST(DesignRegistryTest, FallbackChainAndWildcards)
{
  // --- Test fallback resolution and wildcard handling

  auto &registry = meta::qt::DesignRegistry::instance();

  meta::qt::stock::register_design();
  meta::qt::industrial::register_design();

  // "test_sub" -> "industrial" -> "stock"
  registry.set_fallback("test_sub", "industrial");

  meta::Attribute<float> attr("test_attr", 1.5f);
  attr.metadata().add(meta::keys::ui::widget_type, "SliderFloat");
  attr.metadata().add(meta::keys::constraints::min, 0.f);
  attr.metadata().add(meta::keys::constraints::max, 10.f);

  meta::qt::RowContext ctx;
  auto                *widget = registry.render(&attr, "test_sub", ctx);
  ASSERT_NE(widget, nullptr);
  delete widget;
}

TEST(DesignRegistryTest, CanRenderRejectionFallback)
{
  // --- Test that can_render() returning false falls through to fallback design

  auto &registry = meta::qt::DesignRegistry::instance();

  meta::qt::stock::register_design();

  registry.register_control<float, DummyRejectControl>("rejecting_design",
                                                       "SliderFloat");
  registry.set_fallback("rejecting_design", "stock");

  meta::Attribute<float> attr("float_attr", 2.0f);
  attr.metadata().add(meta::keys::ui::widget_type, "SliderFloat");
  attr.metadata().add(meta::keys::constraints::min, 0.f);
  attr.metadata().add(meta::keys::constraints::max, 10.f);

  meta::qt::RowContext ctx;
  auto                *widget = registry.render(&attr, "rejecting_design", ctx);
  ASSERT_NE(widget, nullptr);
  delete widget;
}

TEST(DesignRegistryTest, CycleDetectionInFallbacks)
{
  // --- Test cycle safety in fallback chains

  auto &registry = meta::qt::DesignRegistry::instance();

  registry.set_fallback("cycle_a", "cycle_b");
  registry.set_fallback("cycle_b", "cycle_a");

  meta::Attribute<double> attr("unsupported", 0.0);
  meta::qt::RowContext    ctx;

  // Should return nullptr gracefully without looping
  auto *widget = registry.render(&attr, "cycle_a", ctx);
  EXPECT_EQ(widget, nullptr);
}

TEST(DesignRegistryTest, SectionFactoryResolution)
{
  // --- Test SectionFactory registration and resolution

  auto &registry = meta::qt::DesignRegistry::instance();

  meta::qt::stock::register_design();
  meta::qt::industrial::register_design();

  auto stock_sec_factory = registry.section_factory("stock");
  ASSERT_TRUE(stock_sec_factory);
  auto *stock_sec = stock_sec_factory("Stock Section");
  ASSERT_NE(stock_sec, nullptr);
  delete stock_sec;

  auto industrial_sec_factory = registry.section_factory("industrial");
  ASSERT_TRUE(industrial_sec_factory);
  auto *industrial_sec = industrial_sec_factory("Industrial Section");
  ASSERT_NE(industrial_sec, nullptr);
  delete industrial_sec;

  // Fallback section resolution
  registry.set_fallback("custom_skin", "industrial");
  auto fallback_sec_factory = registry.section_factory("custom_skin");
  ASSERT_TRUE(fallback_sec_factory);
  auto *fallback_sec = fallback_sec_factory("Custom Skin Section");
  ASSERT_NE(fallback_sec, nullptr);
  delete fallback_sec;
}

TEST(DesignRegistryTest, ContainerWidgetDesignIntegration)
{
  // --- Test rendering full AttributeContainer using
  // ContainerRenderOptions::design

  meta::qt::stock::register_design();
  meta::qt::industrial::register_design();

  meta::AttributeContainer container;
  auto                    *a1 = container.add("param_float", 0.5f);
  a1->metadata().add(meta::keys::ui::widget_type, "SliderFloat");
  a1->metadata().add(meta::keys::constraints::min, 0.f);
  a1->metadata().add(meta::keys::constraints::max, 1.f);
  a1->metadata().add(meta::keys::ui::category, "Parameters");

  auto *a2 = container.add("param_bool", true);
  a2->metadata().add(meta::keys::ui::widget_type, "Toggle");
  a2->metadata().add(meta::keys::ui::category, "Parameters");

  // Render with stock design
  {
    meta::qt::ContainerRenderOptions options;
    options.design = "stock";
    options.category_policy = meta::qt::CategoryPolicy::CP_MERGED;
    auto *widget = meta::qt::render(container, options);
    ASSERT_NE(widget, nullptr);
    delete widget;
  }

  // Render with industrial design
  {
    meta::qt::ContainerRenderOptions options;
    options.design = "industrial";
    options.category_policy = meta::qt::CategoryPolicy::CP_MERGED;
    auto *widget = meta::qt::render(container, options);
    ASSERT_NE(widget, nullptr);
    delete widget;
  }
}
