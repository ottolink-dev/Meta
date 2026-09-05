/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "meta.hpp"

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES

#include "meta/ext/color_gradient/gradient_library.hpp"

namespace {

std::filesystem::path make_temp_dir(const std::string &tag) {
  const auto dir =
      std::filesystem::temp_directory_path() / ("meta_gradient_library_" + tag);
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

meta::Preset make_preset(const std::string &name, float r, float g, float b) {
  return {name, {{0.f, {0.f, 0.f, 0.f, 1.f}}, {1.f, {r, g, b, 1.f}}}};
}

} // namespace

TEST(GradientLibraryTest, AddMakesNamesUnique) {
  meta::GradientLibrary lib;

  EXPECT_EQ(lib.add(make_preset("Fire", 1.f, 0.f, 0.f)), "Fire");
  EXPECT_EQ(lib.add(make_preset("Fire", 0.f, 1.f, 0.f)), "Fire (2)");
  EXPECT_EQ(lib.add(make_preset("  Fire  ", 0.f, 0.f, 1.f)), "Fire (3)");
  EXPECT_EQ(lib.add(make_preset("", 0.f, 0.f, 1.f)), "Gradient");

  EXPECT_EQ(lib.unique_name("Fire"), "Fire (4)");
  EXPECT_EQ(lib.unique_name("Host", {"Host"}), "Host (2)");
  EXPECT_EQ(lib.unique_name("Free"), "Free");

  ASSERT_EQ(lib.presets().size(), 4u);
  EXPECT_TRUE(lib.has("Fire (2)"));
  EXPECT_FALSE(lib.has("Water"));
}

TEST(GradientLibraryTest, AddSortsStopsByPosition) {
  meta::GradientLibrary lib;
  lib.add({"Rev", {{1.f, {1.f, 1.f, 1.f, 1.f}}, {0.f, {0.f, 0.f, 0.f, 1.f}}}});

  const meta::Preset *p = lib.find("Rev");
  ASSERT_NE(p, nullptr);
  EXPECT_FLOAT_EQ(p->stops.front().position, 0.f);
  EXPECT_FLOAT_EQ(p->stops.back().position, 1.f);
  EXPECT_EQ(lib.find("nope"), nullptr);
}

TEST(GradientLibraryTest, UpdateRenameRemove) {
  meta::GradientLibrary lib;
  lib.add(make_preset("A", 1.f, 0.f, 0.f));
  lib.add(make_preset("B", 0.f, 1.f, 0.f));

  const std::vector<meta::Stop> new_stops = {{0.f, {0.f, 0.f, 1.f, 1.f}},
                                             {1.f, {1.f, 1.f, 0.f, 1.f}}};
  EXPECT_TRUE(lib.update("A", new_stops));
  EXPECT_EQ(lib.find("A")->stops, new_stops);
  EXPECT_FALSE(lib.update("Zzz", new_stops));

  EXPECT_FALSE(lib.rename("A", "B"));  // taken
  EXPECT_FALSE(lib.rename("A", "  ")); // empty
  EXPECT_FALSE(lib.rename("Zzz", "C"));
  EXPECT_TRUE(lib.rename("A", "A")); // no-op
  EXPECT_TRUE(lib.rename("A", " C "));
  EXPECT_TRUE(lib.has("C"));
  EXPECT_FALSE(lib.has("A"));

  EXPECT_TRUE(lib.remove("C"));
  EXPECT_FALSE(lib.remove("C"));
  EXPECT_EQ(lib.presets().size(), 1u);

  lib.clear();
  EXPECT_TRUE(lib.presets().empty());
}

TEST(GradientLibraryTest, FavoritesFollowRenameAndRemoval) {
  meta::GradientLibrary lib;
  lib.add(make_preset("A", 1.f, 0.f, 0.f));

  lib.set_favorite("A", true);
  lib.set_favorite("Host preset", true); // not in the library: allowed
  EXPECT_TRUE(lib.is_favorite("A"));
  EXPECT_TRUE(lib.is_favorite("Host preset"));
  EXPECT_EQ(lib.favorites().size(), 2u);

  ASSERT_TRUE(lib.rename("A", "B"));
  EXPECT_FALSE(lib.is_favorite("A"));
  EXPECT_TRUE(lib.is_favorite("B"));

  ASSERT_TRUE(lib.remove("B"));
  EXPECT_FALSE(lib.is_favorite("B"));

  lib.set_favorite("Host preset", false);
  EXPECT_TRUE(lib.favorites().empty());
}

TEST(GradientLibraryTest, ChangedFiresOncePerEffectiveMutation) {
  meta::GradientLibrary lib;
  int count = 0;
  auto conn = lib.changed.subscribe([&count]() { ++count; });

  lib.add(make_preset("A", 1.f, 0.f, 0.f));
  EXPECT_EQ(count, 1);
  lib.set_favorite("A", true);
  EXPECT_EQ(count, 2);
  lib.set_favorite("A", true); // already a favourite
  EXPECT_EQ(count, 2);
  lib.set_sort(meta::GradientSort::Name);
  EXPECT_EQ(count, 3);
  lib.set_sort(meta::GradientSort::Name);
  EXPECT_EQ(count, 3);
  EXPECT_TRUE(lib.remove("A"));
  EXPECT_EQ(count, 4);
  EXPECT_FALSE(lib.remove("A"));
  EXPECT_EQ(count, 4);
}

TEST(GradientLibraryTest, JsonRoundTrip) {
  meta::GradientLibrary lib;
  lib.add(make_preset("A", 1.f, 0.f, 0.f));
  lib.add(make_preset("B", 0.f, 1.f, 0.f));
  lib.set_favorite("B", true);
  lib.set_sort(meta::GradientSort::Luminance);

  const nlohmann::json j = lib.json_to();
  EXPECT_EQ(j["format"], "meta.gradients");
  EXPECT_EQ(j["version"], 1);
  EXPECT_EQ(j["sort"], "luminance");
  ASSERT_EQ(j["gradients"].size(), 2u);

  meta::GradientLibrary copy;
  ASSERT_TRUE(copy.json_from(j));
  EXPECT_EQ(copy.presets(), lib.presets());
  EXPECT_EQ(copy.favorites(), lib.favorites());
  EXPECT_EQ(copy.sort(), meta::GradientSort::Luminance);

  // invalid documents are rejected and leave the state untouched
  EXPECT_FALSE(copy.json_from(nlohmann::json::array()));
  EXPECT_FALSE(copy.json_from(nlohmann::json{{"foo", 1}}));
  EXPECT_EQ(copy.presets().size(), 2u);

  // an empty library document is valid
  meta::GradientLibrary empty;
  ASSERT_TRUE(empty.json_from(meta::GradientLibrary().json_to()));
  EXPECT_TRUE(empty.presets().empty());
}

TEST(GradientLibraryTest, AutosaveAndLoad) {
  const auto dir = make_temp_dir("autosave");
  const auto file = dir / "nested" / "gradients.json";

  {
    meta::GradientLibrary lib;
    lib.set_path(file);
    EXPECT_EQ(lib.path(), file);
    lib.add(make_preset("Saved", 1.f, 0.5f, 0.f));
    lib.set_favorite("Saved", true);
  }
  ASSERT_TRUE(std::filesystem::exists(file));

  meta::GradientLibrary loaded;
  loaded.set_path(file);
  ASSERT_TRUE(loaded.load());
  ASSERT_EQ(loaded.presets().size(), 1u);
  EXPECT_EQ(loaded.presets()[0].name, "Saved");
  EXPECT_TRUE(loaded.is_favorite("Saved"));

  meta::GradientLibrary missing;
  missing.set_path(dir / "missing.json");
  EXPECT_FALSE(missing.load());

  meta::GradientLibrary no_path;
  EXPECT_FALSE(no_path.save());
  no_path.add(make_preset("Memory only", 0.f, 0.f, 1.f));
  EXPECT_EQ(no_path.presets().size(), 1u);

  meta::GradientLibrary manual;
  manual.set_path(dir / "manual.json");
  manual.set_autosave(false);
  EXPECT_FALSE(manual.autosave());
  manual.add(make_preset("Not yet", 0.f, 0.f, 1.f));
  EXPECT_FALSE(std::filesystem::exists(dir / "manual.json"));
  EXPECT_TRUE(manual.save());
  EXPECT_TRUE(std::filesystem::exists(dir / "manual.json"));
}

TEST(GradientLibraryTest, CorruptFileLeavesStateUntouched) {
  const auto dir = make_temp_dir("corrupt");

  meta::GradientLibrary lib;
  lib.add(make_preset("Keep", 1.f, 0.f, 0.f));

  {
    std::ofstream out(dir / "broken.json");
    out << "{ not json";
  }
  lib.set_path(dir / "broken.json");
  EXPECT_FALSE(lib.load());
  EXPECT_EQ(lib.presets().size(), 1u);

  {
    std::ofstream out(dir / "no_gradients.json");
    out << R"({"favorites": []})";
  }
  lib.set_path(dir / "no_gradients.json");
  EXPECT_FALSE(lib.load());
  EXPECT_EQ(lib.presets().size(), 1u);
}

TEST(GradientLibraryTest, ImportPolicy) {
  const auto dir = make_temp_dir("import");

  meta::GradientLibrary source;
  source.add(make_preset("Same", 1.f, 0.f, 0.f));
  source.add(make_preset("Conflict", 0.f, 1.f, 0.f));
  source.add(make_preset("New", 0.f, 0.f, 1.f));
  source.set_favorite("New", true);
  ASSERT_TRUE(source.export_file(dir / "export.json", source.presets()));

  nlohmann::json exported;
  std::ifstream(dir / "export.json") >> exported;
  EXPECT_FALSE(exported.contains("favorites"));
  EXPECT_FALSE(exported.contains("sort"));
  EXPECT_EQ(exported["format"], "meta.gradients");

  meta::GradientLibrary lib;
  lib.add(make_preset("Same", 1.f, 0.f, 0.f));
  lib.add(make_preset("Conflict", 0.2f, 0.2f, 0.2f));

  const auto report = lib.import_file(dir / "export.json");
  EXPECT_TRUE(report.ok);
  EXPECT_EQ(report.added, 1u);
  EXPECT_EQ(report.renamed, 1u);
  EXPECT_EQ(report.skipped, 1u);
  EXPECT_TRUE(lib.has("New"));
  EXPECT_TRUE(lib.has("Conflict (2)"));
  EXPECT_EQ(lib.presets().size(), 4u);
  EXPECT_FALSE(lib.is_favorite("New"));

  EXPECT_FALSE(lib.import_file(dir / "nope.json").ok);
  EXPECT_EQ(lib.presets().size(), 4u);
}

TEST(GradientLibraryTest, ParsesHesiodFileShapes) {
  // Hesiod data/color_gradients/<stem>.json: Meta's ColorGradient::json_to
  // shape, no name -> the fallback (file stem) is used
  const auto per_file = nlohmann::json::parse(R"({
    "label": "gradient", "type": 3,
    "value": [
      {"color": [0.1, 0.2, 0.3, 1.0], "position": 0.0},
      {"color": [0.5, 0.6, 0.7, 1.0], "position": 1.0}
    ]})");
  auto parsed = meta::parse_gradient_file(per_file, "051c4a");
  ASSERT_TRUE(parsed.has_value());
  ASSERT_EQ(parsed->size(), 1u);
  EXPECT_EQ((*parsed)[0].name, "051c4a");
  EXPECT_NEAR((*parsed)[0].stops[1].color[2], 0.7f, 1e-6f);

  // Hesiod data/color_gradient.json: collection with 0-255 colours
  const auto collection = nlohmann::json::parse(R"({
    "gradients": [
      {"name": "Snow", "stops": [
        {"position": 0.0, "color": [200, 220, 255, 255]},
        {"position": 1.0, "color": [255, 255, 255, 255]}]}
    ]})");
  parsed = meta::parse_gradient_file(collection);
  ASSERT_TRUE(parsed.has_value());
  ASSERT_EQ(parsed->size(), 1u);
  EXPECT_EQ((*parsed)[0].name, "Snow");
  EXPECT_NEAR((*parsed)[0].stops[0].color[0], 200.f / 255.f, 1e-6f);
  EXPECT_NEAR((*parsed)[0].stops[0].color[3], 1.f, 1e-6f);

  // bare array without names -> numbered fallbacks; RGB gets alpha 1;
  // unsorted stops are sorted
  const auto bare = nlohmann::json::parse(R"([
    {"stops": [{"position": 1.0, "color": [0, 0, 1]},
               {"position": 0.0, "color": [1, 0, 0]}]},
    {"stops": [{"position": 0.0, "color": [0, 1, 0]},
               {"position": 1.0, "color": [0, 0, 0]}]}
  ])");
  parsed = meta::parse_gradient_file(bare, "Import");
  ASSERT_TRUE(parsed.has_value());
  ASSERT_EQ(parsed->size(), 2u);
  EXPECT_EQ((*parsed)[0].name, "Import 1");
  EXPECT_EQ((*parsed)[1].name, "Import 2");
  EXPECT_FLOAT_EQ((*parsed)[0].stops[0].position, 0.f);
  EXPECT_FLOAT_EQ((*parsed)[0].stops[0].color[0], 1.f);
  EXPECT_FLOAT_EQ((*parsed)[0].stops[0].color[3], 1.f);

  // entries with fewer than two valid stops are dropped; garbage is rejected
  const auto degenerate = nlohmann::json::parse(
      R"({"name": "x", "stops": [{"position": 0.0, "color": [1, 1, 1, 1]}]})");
  EXPECT_FALSE(meta::parse_gradient_file(degenerate).has_value());
  EXPECT_FALSE(meta::parse_gradient_file(nlohmann::json(42)).has_value());
  EXPECT_FALSE(meta::parse_gradient_file(nlohmann::json::object()).has_value());
}

TEST(GradientLibraryTest, SortNamesRoundTrip) {
  using meta::GradientSort;
  for (GradientSort s : {GradientSort::Default, GradientSort::Name,
                         GradientSort::Luminance, GradientSort::Hue}) {
    const auto back = meta::gradient_sort_from_string(meta::to_string(s));
    ASSERT_TRUE(back.has_value());
    EXPECT_EQ(*back, s);
  }
  EXPECT_FALSE(meta::gradient_sort_from_string("bogus").has_value());
}

#endif // META_ENABLE_COLOR_GRADIENT_TYPES
