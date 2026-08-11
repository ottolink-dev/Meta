/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <algorithm>
#include <cstring>

#include "meta/ext/array/array.hpp"
#include "meta/logger.hpp"

namespace meta
{

void Array::json_from(nlohmann::json const &json)
{
  shape = {0, 0};
  vector.clear();

  if (json.contains("shape"))
  {
    auto const &s = json["shape"];
    if (s.contains("x") && s.contains("y"))
    {
      shape.x = s["x"].get<int>();
      shape.y = s["y"].get<int>();
    }
  }

  if (json.contains("vector"))
  {
    auto const &v = json["vector"];
    if (v.is_binary())
    {
      auto const &bytes = v.get_binary();
      if (bytes.size() % sizeof(float) == 0)
      {
        vector.resize(bytes.size() / sizeof(float));
        std::memcpy(vector.data(), bytes.data(), bytes.size());
      }
    }
    else if (v.is_array())
    {
      vector = v.get<std::vector<float>>();
    }
  }

  size_t const expected_size = static_cast<size_t>(std::max(0, shape.x)) *
                               static_cast<size_t>(std::max(0, shape.y));
  if (vector.size() != expected_size)
  {
    Logger::log()->warn("Array size mismatch: shape ({}x{}) expects {} "
                        "elements, but data has {} elements.",
                        shape.x,
                        shape.y,
                        expected_size,
                        vector.size());
    vector.resize(expected_size, 0.0f);
  }
}

nlohmann::json Array::json_to() const
{
  nlohmann::json j;
  j["shape"] = {{"x", shape.x}, {"y", shape.y}};

  std::vector<uint8_t> bytes(vector.size() * sizeof(float));
  if (!bytes.empty())
  {
    std::memcpy(bytes.data(), vector.data(), bytes.size());
  }
  j["vector"] = nlohmann::json::binary(bytes);

  return j;
}

} // namespace meta
