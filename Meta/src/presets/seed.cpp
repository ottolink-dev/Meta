/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/core/attribute_container.hpp"
#include "meta/metadata/keys.hpp"

namespace meta::presets
{

Attribute<int> &seed(AttributeContainer &c,
                     std::string_view    key,
                     std::string_view    label,
                     int                 value)
{
  auto *a = c.add(std::string(key), value);
  a->value() = value;

  auto &m = a->metadata();

  m.add(keys::ui::widget_type, "SliderInt");
  m.add(keys::ui::label, std::string(label));
  m.add(keys::constraints::min, 0);
  m.add(keys::constraints::max, INT_MAX);
  m.add(keys::constraints::step, 1);
  m.add("ui.plus_minus", true);

  return *a;
}

} // namespace meta::presets
