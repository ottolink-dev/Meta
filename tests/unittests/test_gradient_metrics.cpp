/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <gtest/gtest.h>

#include "meta.hpp"

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES

#include "meta/ext/color_gradient/gradient_metrics.hpp"

namespace {

meta::Stop stop(float t, float r, float g, float b, float a = 1.f) {
  return {t, {r, g, b, a}};
}

} // namespace

TEST(GradientMetricsTest, StopAndPresetEquality) {
  const meta::Preset a{"x",
                       {stop(0.f, 0.f, 0.f, 0.f), stop(1.f, 1.f, 1.f, 1.f)}};
  meta::Preset b = a;
  EXPECT_EQ(a, b);
  b.stops[1].color[0] = 0.5f;
  EXPECT_NE(a, b);
  b = a;
  b.name = "y";
  EXPECT_NE(a, b);
}

TEST(GradientMetricsTest, SampleInterpolatesAndClamps) {
  // deliberately unsorted
  const std::vector<meta::Stop> stops = {stop(1.f, 1.f, 1.f, 1.f, 1.f),
                                         stop(0.f, 0.f, 0.f, 0.f, 0.f)};

  const auto mid = meta::sample_gradient(stops, 0.5f);
  for (int k = 0; k < 4; ++k)
    EXPECT_NEAR(mid[k], 0.5f, 1e-5f);

  EXPECT_FLOAT_EQ(meta::sample_gradient(stops, -1.f)[0], 0.f);
  EXPECT_FLOAT_EQ(meta::sample_gradient(stops, 2.f)[0], 1.f);

  const auto empty = meta::sample_gradient({}, 0.3f);
  EXPECT_FLOAT_EQ(empty[0], 0.f);
  EXPECT_FLOAT_EQ(empty[3], 1.f);
}

TEST(GradientMetricsTest, SampleHoldsEndColorsOutsideStopRange) {
  const std::vector<meta::Stop> stops = {stop(0.25f, 1.f, 0.f, 0.f),
                                         stop(0.75f, 0.f, 0.f, 1.f)};
  EXPECT_FLOAT_EQ(meta::sample_gradient(stops, 0.1f)[0], 1.f);
  EXPECT_FLOAT_EQ(meta::sample_gradient(stops, 0.9f)[2], 1.f);
  EXPECT_NEAR(meta::sample_gradient(stops, 0.5f)[0], 0.5f, 1e-5f);
}

TEST(GradientMetricsTest, LuminanceOrdersDarkToLight) {
  const std::vector<meta::Stop> black = {stop(0.f, 0.f, 0.f, 0.f),
                                         stop(1.f, 0.f, 0.f, 0.f)};
  const std::vector<meta::Stop> grey = {stop(0.f, 0.5f, 0.5f, 0.5f),
                                        stop(1.f, 0.5f, 0.5f, 0.5f)};
  const std::vector<meta::Stop> white = {stop(0.f, 1.f, 1.f, 1.f),
                                         stop(1.f, 1.f, 1.f, 1.f)};
  const std::vector<meta::Stop> ramp = {stop(0.f, 0.f, 0.f, 0.f),
                                        stop(1.f, 1.f, 1.f, 1.f)};

  EXPECT_LT(meta::gradient_luminance(black), meta::gradient_luminance(grey));
  EXPECT_LT(meta::gradient_luminance(grey), meta::gradient_luminance(white));
  EXPECT_NEAR(meta::gradient_luminance(white), 1.f, 1e-5f);
  EXPECT_NEAR(meta::gradient_luminance(ramp), 0.5f, 0.02f);
  EXPECT_FLOAT_EQ(meta::gradient_luminance({}), 0.f);
}

TEST(GradientMetricsTest, HueOfPrimaries) {
  const std::vector<meta::Stop> red = {stop(0.f, 1.f, 0.f, 0.f),
                                       stop(1.f, 1.f, 0.f, 0.f)};
  const std::vector<meta::Stop> green = {stop(0.f, 0.f, 1.f, 0.f),
                                         stop(1.f, 0.f, 1.f, 0.f)};
  const std::vector<meta::Stop> blue = {stop(0.f, 0.f, 0.f, 1.f),
                                        stop(1.f, 0.f, 0.f, 1.f)};

  const float h_red = meta::gradient_hue(red);
  EXPECT_TRUE(h_red < 1.f || h_red > 359.f) << h_red;
  EXPECT_NEAR(meta::gradient_hue(green), 120.f, 0.5f);
  EXPECT_NEAR(meta::gradient_hue(blue), 240.f, 0.5f);
}

TEST(GradientMetricsTest, HueAveragesAcrossWrapAround) {
  // 350 deg -> 10 deg through red: the circular mean sits near 0, not 180
  const std::vector<meta::Stop> stops = {stop(0.f, 1.f, 0.f, 1.f / 6.f),
                                         stop(1.f, 1.f, 1.f / 6.f, 0.f)};
  const float h = meta::gradient_hue(stops);
  EXPECT_TRUE(h < 5.f || h > 355.f) << h;
}

TEST(GradientMetricsTest, AchromaticGradientHasNoHue) {
  const std::vector<meta::Stop> ramp = {stop(0.f, 0.f, 0.f, 0.f),
                                        stop(1.f, 1.f, 1.f, 1.f)};
  EXPECT_FLOAT_EQ(meta::gradient_hue(ramp), -1.f);
  EXPECT_FLOAT_EQ(meta::gradient_hue({}), -1.f);
}

#endif // META_ENABLE_COLOR_GRADIENT_TYPES
