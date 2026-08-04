/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace meta
{

/// A structure representing an array with dimensions (shape) and flat data (vector).
struct Array
{
  /// Array dimensions (e.g. width, height).
  glm::ivec2 shape{0, 0};

  /// Flat data.
  std::vector<float> vector;

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
};

} // namespace meta

#include "meta/ext/array/array.inl"
