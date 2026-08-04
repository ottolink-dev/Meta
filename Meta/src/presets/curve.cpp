/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/presets/curve.hpp"
#include "meta/core/attribute_container.hpp"
#include "meta/metadata/keys.hpp"

namespace meta::presets
{

Attribute<std::vector<float>> &curve(AttributeContainer &c, std::string_view key,
                                     std::string_view label, std::vector<float> value,
                                     float vmin, float vmax)
{
  auto *a = c.add(std::string(key), std::move(value));
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "CurveEditor");
  m.add(keys::ui::label, std::string(label));
  m.add(keys::ui::min_y, vmin);
  m.add(keys::ui::max_y, vmax);
  return *a;
}

} // namespace meta::presets
