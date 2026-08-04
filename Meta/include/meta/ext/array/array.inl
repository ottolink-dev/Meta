/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <string>
#include <sstream>

#include "meta/type/attribute_traits.hpp"
#include "meta/type/type_name.hpp"

namespace meta
{

/// Typename for Array.
template <> struct TypeName<meta::Array>
{
  static constexpr std::string_view name = "meta::Array";
};

/// Traits specialization for Array serialization and formatting.
template <> struct AttributeTraits<Array>
{
  static std::string to_string(const Array &v)
  {
    std::ostringstream oss;
    oss << "shape: (" << v.shape.x << ", " << v.shape.y << "), vector size: " << v.vector.size();
    return oss.str();
  }

  static nlohmann::json json_to(const Array &v) { return v.json_to(); }

  static Array json_from(const nlohmann::json &j)
  {
    Array v;
    v.json_from(j);
    return v;
  }
};

} // namespace meta
