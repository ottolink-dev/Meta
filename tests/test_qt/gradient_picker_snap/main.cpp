// Offscreen snapshot harness for GradientPicker (issue #50). Renders the
// widget with host presets plus an isolated gradient library (favourites,
// user entries, each sort key) to PNG files for visual review. Not part of
// the test suite; run with QT_QPA_PLATFORM=offscreen.
#include <filesystem>
#include <vector>

#include <QApplication>
#include <QDir>

#include "meta/ext/color_gradient/gradient_library.hpp"
#include "meta_qt/widgets/gradient_picker.hpp"

using meta::GradientLibrary;
using meta::GradientSort;
using meta::Preset;
using meta::Stop;

static Preset make(const std::string &name, std::vector<Stop> stops) {
  return {name, std::move(stops)};
}

static void snap(QWidget &w, const QString &path) {
  w.resize(320, 260);
  w.grab().save(path);
}

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  const QString dir = argc > 1 ? argv[1] : ".";
  QDir().mkpath(dir);

  // Isolated library so the snapshot never touches a real per-user file
  GradientLibrary &lib = GradientLibrary::instance();
  lib.set_path(std::filesystem::temp_directory_path() /
               "meta_gradient_picker_snap" / "gradients.json");
  lib.clear();

  const std::vector<Preset> host = {
      make("Snow",
           {{0.f, {0.78f, 0.86f, 1.f, 1.f}}, {1.f, {1.f, 1.f, 1.f, 1.f}}}),
      make("Sand",
           {{0.f, {0.76f, 0.7f, 0.5f, 1.f}}, {1.f, {0.94f, 0.9f, 0.7f, 1.f}}}),
      make("Grass", {{0.f, {0.12f, 0.27f, 0.12f, 1.f}},
                     {1.f, {0.5f, 0.75f, 0.3f, 1.f}}}),
      make("Ocean",
           {{0.f, {0.f, 0.05f, 0.25f, 1.f}}, {1.f, {0.2f, 0.6f, 0.9f, 1.f}}}),
      make("Lava", {{0.f, {0.1f, 0.f, 0.f, 1.f}},
                    {0.6f, {0.9f, 0.2f, 0.f, 1.f}},
                    {1.f, {1.f, 0.9f, 0.3f, 1.f}}}),
      make("Greys",
           {{0.f, {0.f, 0.f, 0.f, 1.f}}, {1.f, {1.f, 1.f, 1.f, 1.f}}})};

  lib.add(make("My sunset", {{0.f, {0.2f, 0.f, 0.3f, 1.f}},
                             {0.5f, {0.9f, 0.3f, 0.2f, 1.f}},
                             {1.f, {1.f, 0.8f, 0.4f, 1.f}}}));
  lib.add(make("Mint", {{0.f, {0.f, 0.3f, 0.25f, 1.f}},
                        {1.f, {0.6f, 1.f, 0.85f, 1.f}}}));
  lib.set_favorite("Ocean", true);
  lib.set_favorite("Mint", true);

  std::vector<Stop> stops = {{0.f, {0.1f, 0.1f, 0.3f, 1.f}},
                             {0.5f, {0.9f, 0.5f, 0.1f, 1.f}},
                             {1.f, {1.f, 1.f, 0.8f, 1.f}}};

  for (GradientSort sort : {GradientSort::Default, GradientSort::Name,
                            GradientSort::Luminance, GradientSort::Hue}) {
    lib.set_sort(sort);
    meta::qt::GradientPicker picker(stops, host);
    snap(picker,
         dir + "/sort_" +
             QString::fromStdString(std::string(meta::to_string(sort))) +
             ".png");
  }

  // No host presets, empty library: bar + toolbar only
  lib.clear();
  meta::qt::GradientPicker bare(stops, {});
  snap(bare, dir + "/empty.png");

  return 0;
}
