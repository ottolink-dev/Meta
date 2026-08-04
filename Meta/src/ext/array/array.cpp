/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/ext/array/array.hpp"
#include <cstring>

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
