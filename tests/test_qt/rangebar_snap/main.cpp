// Offscreen snapshot harness for RangeBar (issues #23 / #20). Renders the
// widget in several histogram configurations to PNG files for visual review.
// Not part of the test suite; run with QT_QPA_PLATFORM=offscreen.
#include <cmath>
#include <vector>

#include <QApplication>
#include <QDir>

#include "meta_qt/widgets/range_bar.hpp"

using meta::qt::RangeBar;

static void snap(QWidget &w, const QString &path)
{
  w.resize(420, 44);
  w.grab().save(path);
}

int main(int argc, char **argv)
{
  QApplication  app(argc, argv);
  const QString dir = argc > 1 ? argv[1] : ".";
  QDir().mkpath(dir);

  // Bell-ish histogram over the full domain [0, 1]
  std::vector<float> x, y;
  const int          n = 60;
  for (int i = 0; i < n; ++i)
  {
    const float cx = (float(i) + 0.5f) / float(n);
    x.push_back(cx);
    y.push_back(std::exp(-40.f * (cx - 0.55f) * (cx - 0.55f)) +
                0.3f * std::exp(-200.f * (cx - 0.15f) * (cx - 0.15f)));
  }

  glm::vec2 v1{0.3f, 0.8f};
  RangeBar  full(v1, 0.f, 1.f, 2);
  full.set_histogram(x, y);
  snap(full, dir + "/full_domain.png");

  // Partial-domain histogram: data only covers [0.5, 1.0] of a [0, 1] domain.
  // Pre-fix this was stretched over the whole widget.
  std::vector<float> xp, yp;
  for (int i = 0; i < n; ++i)
  {
    const float cx = 0.5f + 0.5f * (float(i) + 0.5f) / float(n);
    xp.push_back(cx);
    yp.push_back(1.f + std::sin(12.f * cx));
  }
  glm::vec2 v2{0.4f, 0.7f};
  RangeBar  partial(v2, 0.f, 1.f, 2);
  partial.set_histogram(xp, yp);
  snap(partial, dir + "/partial_domain.png");

  // Provider ran but returned nothing → "no histogram data" hint
  glm::vec2 v3{0.2f, 0.9f};
  RangeBar  empty(v3, 0.f, 1.f, 2);
  empty.set_histogram({}, {});
  snap(empty, dir + "/empty_provider.png");

  // No histogram at all → plain bar, no hint
  glm::vec2 v4{0.2f, 0.9f};
  RangeBar  plain(v4, 0.f, 1.f, 2);
  snap(plain, dir + "/no_histogram.png");

  return 0;
}
