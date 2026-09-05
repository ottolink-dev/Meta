/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <algorithm>
#include <cmath>
#include <numbers>

#include "meta/ext/color_gradient/gradient_metrics.hpp"

namespace meta {

namespace {

std::vector<Stop> sorted_stops(const std::vector<Stop> &stops) {
  std::vector<Stop> sorted = stops;
  std::stable_sort(
      sorted.begin(), sorted.end(),
      [](const Stop &a, const Stop &b) { return a.position < b.position; });
  return sorted;
}

// Expects sorted, non-empty stops.
std::array<float, 4> sample_sorted(const std::vector<Stop> &sorted, float t) {
  t = std::clamp(t, 0.f, 1.f);

  if (t <= sorted.front().position)
    return sorted.front().color;
  if (t >= sorted.back().position)
    return sorted.back().color;

  for (std::size_t i = 1; i < sorted.size(); ++i) {
    if (t > sorted[i].position)
      continue;

    const Stop &a = sorted[i - 1];
    const Stop &b = sorted[i];
    const float span = b.position - a.position;
    const float u = span > 0.f ? (t - a.position) / span : 1.f;

    std::array<float, 4> out{};
    for (std::size_t k = 0; k < 4; ++k)
      out[k] = a.color[k] + (b.color[k] - a.color[k]) * u;
    return out;
  }

  return sorted.back().color;
}

float sample_position(int i, int samples) {
  return samples == 1 ? 0.5f : float(i) / float(samples - 1);
}

// HSV hue in degrees [0, 360) and saturation in [0, 1].
void hue_saturation(const std::array<float, 4> &c, float &hue, float &sat) {
  const float r = std::clamp(c[0], 0.f, 1.f);
  const float g = std::clamp(c[1], 0.f, 1.f);
  const float b = std::clamp(c[2], 0.f, 1.f);
  const float mx = std::max({r, g, b});
  const float mn = std::min({r, g, b});
  const float d = mx - mn;

  hue = 0.f;
  sat = mx > 0.f ? d / mx : 0.f;
  if (d <= 1e-6f)
    return;

  float h;
  if (mx == r)
    h = std::fmod((g - b) / d, 6.f);
  else if (mx == g)
    h = (b - r) / d + 2.f;
  else
    h = (r - g) / d + 4.f;

  h *= 60.f;
  if (h < 0.f)
    h += 360.f;
  hue = h;
}

} // namespace

std::array<float, 4> sample_gradient(const std::vector<Stop> &stops, float t) {
  if (stops.empty())
    return {0.f, 0.f, 0.f, 1.f};
  return sample_sorted(sorted_stops(stops), t);
}

float gradient_luminance(const std::vector<Stop> &stops, int samples) {
  if (stops.empty() || samples <= 0)
    return 0.f;

  const std::vector<Stop> sorted = sorted_stops(stops);
  float sum = 0.f;

  for (int i = 0; i < samples; ++i) {
    const auto c = sample_sorted(sorted, sample_position(i, samples));
    sum += 0.2126f * std::clamp(c[0], 0.f, 1.f) +
           0.7152f * std::clamp(c[1], 0.f, 1.f) +
           0.0722f * std::clamp(c[2], 0.f, 1.f);
  }

  return sum / float(samples);
}

float gradient_hue(const std::vector<Stop> &stops, int samples) {
  if (stops.empty() || samples <= 0)
    return -1.f;

  const std::vector<Stop> sorted = sorted_stops(stops);
  double sx = 0.0;
  double sy = 0.0;
  double weight = 0.0;

  for (int i = 0; i < samples; ++i) {
    float hue = 0.f;
    float sat = 0.f;
    hue_saturation(sample_sorted(sorted, sample_position(i, samples)), hue,
                   sat);

    const double rad = double(hue) * std::numbers::pi / 180.0;
    sx += sat * std::cos(rad);
    sy += sat * std::sin(rad);
    weight += sat;
  }

  if (weight < 1e-4)
    return -1.f;

  double deg = std::atan2(sy, sx) * 180.0 / std::numbers::pi;
  if (deg < 0.0)
    deg += 360.0;
  if (deg >= 360.0)
    deg -= 360.0;
  return float(deg);
}

} // namespace meta
