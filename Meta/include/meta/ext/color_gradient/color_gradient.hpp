/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <array>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace meta
{

/// A color stop in a gradient.
struct Stop
{
  /// Position in the range [0, 1].
  float position;

  /// RGBA color.
  std::array<float, 4> color;
};

/// A named color gradient preset.
struct Preset
{
  /// Preset name.
  std::string name;

  /// Gradient stops.
  std::vector<Stop> stops;
};

/// Editable color gradient. Presets are deliberately NOT part of this value
/// type: they are host configuration, carried in attribute metadata as a
/// GradientPresets entry (keys::ui::presets), so that deserializing a value
/// cannot clobber the preset library installed at setup time.
class ColorGradient
{
public:
  /// Constructs a default black-to-white gradient.
  ColorGradient() = default;

  /**
   * @brief Deserializes the object from a JSON representation.
   *
   * @param json Input JSON data used to restore the object state.
   */
  void json_from(nlohmann::json const &json);

  /**
   * @brief Serializes the object to a JSON representation.
   *
   * @return nlohmann::json JSON object representing the current state.
   */
  nlohmann::json json_to() const;

  /// Sets the gradient stops.
  void set_value(const std::vector<Stop> &new_value);

  /// Returns the gradient stops.
  const std::vector<Stop> &value() const;

  /// Returns the gradient stops.
  std::vector<Stop> &value();

private:
  std::vector<Stop> value_ = {{0.f, {0.f, 0.f, 0.f, 1.f}},
                              {1.f, {1.f, 1.f, 1.f, 1.f}}};
};

/// Preset library for a gradient attribute, installed by the host into
/// attribute metadata under keys::ui::presets. Runtime configuration, not
/// document state: never serialized (mirrors meta::DataProvider).
struct GradientPresets
{
  /// Available presets.
  std::vector<Preset> presets;
};

} // namespace meta

#include "meta/ext/color_gradient/color_gradient.inl"