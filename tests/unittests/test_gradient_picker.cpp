/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <filesystem>

#include <gtest/gtest.h>

#include "meta.hpp"

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES

#include <QCoreApplication>
#include <QPushButton>

#include "meta/ext/color_gradient/gradient_library.hpp"
#include "meta_qt/widgets/gradient_picker.hpp"

namespace {

std::vector<meta::Preset> host_presets() {
  return {
      {"Host B", {{0.f, {0.f, 0.f, 0.f, 1.f}}, {1.f, {1.f, 0.f, 0.f, 1.f}}}},
      {"Host A", {{0.f, {0.f, 0.f, 0.f, 1.f}}, {1.f, {0.f, 0.f, 1.f, 1.f}}}}};
}

meta::Preset lib_preset(const std::string &name) {
  return {name, {{0.f, {0.f, 0.f, 0.f, 1.f}}, {1.f, {0.f, 1.f, 0.f, 1.f}}}};
}

// Points the process-wide library at a scratch file and empties it, so the
// tests never touch a real per-user library.
struct IsolatedLibrary {
  IsolatedLibrary() {
    const auto dir =
        std::filesystem::temp_directory_path() / "meta_gradient_picker_test";
    std::filesystem::remove_all(dir);

    auto &lib = meta::GradientLibrary::instance();
    lib.set_path(dir / "gradients.json");
    lib.clear();
    lib.set_sort(meta::GradientSort::Default);
  }

  ~IsolatedLibrary() {
    auto &lib = meta::GradientLibrary::instance();
    lib.clear();
    lib.set_sort(meta::GradientSort::Default);
  }
};

// Library notifications rebuild the grid through a queued call.
void flush() {
  QCoreApplication::sendPostedEvents();
  QCoreApplication::processEvents();
}

int swatch_count(const meta::qt::GradientPicker &picker) {
  return static_cast<int>(picker.findChildren<QPushButton *>().size());
}

} // namespace

TEST(GradientPickerTest, MergesHostAndLibraryPresets) {
  IsolatedLibrary isolated;
  meta::GradientLibrary::instance().add(lib_preset("Lib"));

  std::vector<meta::Stop> stops = {{0.f, {0.f, 0.f, 0.f, 1.f}},
                                   {1.f, {1.f, 1.f, 1.f, 1.f}}};
  meta::qt::GradientPicker picker(stops, host_presets());

  EXPECT_EQ(picker.entry_names(),
            (std::vector<std::string>{"Host B", "Host A", "Lib"}));
  EXPECT_EQ(swatch_count(picker), 3);

  int user_count = 0;
  for (auto *button : picker.findChildren<QPushButton *>())
    if (button->property("preset_user").toBool())
      ++user_count;
  EXPECT_EQ(user_count, 1);
}

TEST(GradientPickerTest, FavoritesPinnedFirstThenSortKey) {
  IsolatedLibrary isolated;
  auto &lib = meta::GradientLibrary::instance();
  lib.add(lib_preset("Lib"));

  std::vector<meta::Stop> stops = {{0.f, {0.f, 0.f, 0.f, 1.f}},
                                   {1.f, {1.f, 1.f, 1.f, 1.f}}};
  meta::qt::GradientPicker picker(stops, host_presets());

  lib.set_favorite("Lib", true);
  flush();
  EXPECT_EQ(picker.entry_names(),
            (std::vector<std::string>{"Lib", "Host B", "Host A"}));

  lib.set_sort(meta::GradientSort::Name);
  flush();
  EXPECT_EQ(picker.entry_names(),
            (std::vector<std::string>{"Lib", "Host A", "Host B"}));

  lib.set_favorite("Lib", false);
  flush();
  EXPECT_EQ(picker.entry_names(),
            (std::vector<std::string>{"Host A", "Host B", "Lib"}));
}

TEST(GradientPickerTest, LuminanceSortGoesDarkToLight) {
  IsolatedLibrary isolated;
  auto &lib = meta::GradientLibrary::instance();
  lib.add(
      {"Bright", {{0.f, {1.f, 1.f, 1.f, 1.f}}, {1.f, {1.f, 1.f, 1.f, 1.f}}}});
  lib.add(
      {"Dark", {{0.f, {0.f, 0.f, 0.f, 1.f}}, {1.f, {0.1f, 0.1f, 0.1f, 1.f}}}});
  lib.set_sort(meta::GradientSort::Luminance);

  std::vector<meta::Stop> stops = {{0.f, {0.f, 0.f, 0.f, 1.f}},
                                   {1.f, {1.f, 1.f, 1.f, 1.f}}};
  meta::qt::GradientPicker picker(stops, {});

  EXPECT_EQ(picker.entry_names(), (std::vector<std::string>{"Dark", "Bright"}));
}

TEST(GradientPickerTest, SaveCurrentAsPresetAvoidsHostNames) {
  IsolatedLibrary isolated;
  auto &lib = meta::GradientLibrary::instance();

  std::vector<meta::Stop> stops = {{1.f, {1.f, 1.f, 0.f, 1.f}},
                                   {0.f, {0.f, 0.f, 0.f, 1.f}}};
  meta::qt::GradientPicker picker(stops, host_presets());

  EXPECT_EQ(picker.save_current_as_preset("Mine"), "Mine");
  EXPECT_EQ(picker.save_current_as_preset("Mine"), "Mine (2)");
  EXPECT_EQ(picker.save_current_as_preset("Host A"), "Host A (2)");

  const meta::Preset *saved = lib.find("Mine");
  ASSERT_NE(saved, nullptr);
  ASSERT_EQ(saved->stops.size(), 2u);
  EXPECT_FLOAT_EQ(saved->stops[0].position, 0.f); // stored sorted
  EXPECT_FLOAT_EQ(saved->stops[1].color[0], 1.f);

  flush();
  EXPECT_EQ(picker.entry_names().size(), 5u);
  EXPECT_EQ(swatch_count(picker), 5);
}

TEST(GradientPickerTest, LibraryChangesRebuildTheGrid) {
  IsolatedLibrary isolated;
  auto &lib = meta::GradientLibrary::instance();

  std::vector<meta::Stop> stops = {{0.f, {0.f, 0.f, 0.f, 1.f}},
                                   {1.f, {1.f, 1.f, 1.f, 1.f}}};
  meta::qt::GradientPicker picker(stops, host_presets());
  EXPECT_EQ(swatch_count(picker), 2);

  lib.add(lib_preset("One"));
  lib.add(lib_preset("Two"));
  flush();
  EXPECT_EQ(swatch_count(picker), 4);

  ASSERT_TRUE(lib.remove("One"));
  flush();
  EXPECT_EQ(swatch_count(picker), 3);
  EXPECT_EQ(picker.entry_names().back(), "Two");
}

#endif // META_ENABLE_COLOR_GRADIENT_TYPES
