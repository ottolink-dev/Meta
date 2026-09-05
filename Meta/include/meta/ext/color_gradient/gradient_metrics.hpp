/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <array>
#include <vector>

#include "meta/ext/color_gradient/color_gradient.hpp"

namespace meta {

/**
 * @brief Colour of a gradient at position `t`.
 *
 * Linear interpolation between the two stops bracketing `t` (clamped to
 * [0, 1]); positions outside the stop range hold the end colours. Stops need
 * not be sorted. An empty gradient samples as opaque black.
 */
std::array<float, 4> sample_gradient(const std::vector<Stop> &stops, float t);

/**
 * @brief Mean Rec.709 luma of the gradient in [0, 1].
 *
 * Averages `samples` evenly spaced samples of `0.2126 R + 0.7152 G + 0.0722 B`
 * on the stored (sRGB-encoded) components; alpha is ignored. Used for the
 * "Luminance" sort of preset grids.
 */
float gradient_luminance(const std::vector<Stop> &stops, int samples = 32);

/**
 * @brief Dominant hue of the gradient in degrees [0, 360).
 *
 * Saturation-weighted circular mean of the HSV hue over `samples` evenly
 * spaced samples. Returns -1 when the gradient is achromatic (total
 * saturation weight below 1e-4), so hue-sorted lists can push greys last.
 */
float gradient_hue(const std::vector<Stop> &stops, int samples = 32);

} // namespace meta
