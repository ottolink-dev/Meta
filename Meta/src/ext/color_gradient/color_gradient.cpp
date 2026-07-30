/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/ext/color_gradient/color_gradient.hpp"

namespace meta
{

void ColorGradient::json_from(nlohmann::json const &json)
{
  value_.clear();

  if (json.contains("value"))
  {
    for (const auto &j : json["value"].items())
    {
      if (j.value().contains("position") && j.value().contains("color"))
      {
        Stop s{j.value()["position"], j.value()["color"]};
        value_.push_back(s);
      }
    }
  }
}

nlohmann::json ColorGradient::json_to() const
{
  nlohmann::json j;
  for (auto &v : value_)
    j["value"].push_back({{"position", v.position}, {"color", v.color}});

  return j;
}

void ColorGradient::set_value(const std::vector<Stop> &new_value)
{
  value_ = new_value;
}

const std::vector<Stop> &ColorGradient::value() const { return value_; }

std::vector<Stop> &ColorGradient::value() { return value_; }

} // namespace meta
