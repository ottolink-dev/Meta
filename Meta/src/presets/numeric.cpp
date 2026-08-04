/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/presets/numeric.hpp"
#include "meta/core/attribute_container.hpp"
#include "meta/metadata/keys.hpp"

namespace meta::presets
{

Attribute<float> &slider_float(AttributeContainer &c, std::string_view key,
                               std::string_view label, float value, float vmin, float vmax,
                               std::string_view format, bool log_scale)
{
  auto *a = c.add(std::string(key), value);
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "SliderFloat");
  m.add(keys::ui::label, std::string(label));
  m.add(keys::ui::format, std::string(format));
  m.add(keys::constraints::min, vmin);
  m.add(keys::constraints::max, vmax);
  if (log_scale)
    m.add(keys::ui::log_scale, true);
  return *a;
}

Attribute<int> &slider_int(AttributeContainer &c, std::string_view key, std::string_view label,
                           int value, int vmin, int vmax, std::string_view format)
{
  auto *a = c.add(std::string(key), value);
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "SliderInt");
  m.add(keys::ui::label, std::string(label));
  m.add(keys::ui::format, std::string(format));
  m.add(keys::constraints::min, vmin);
  m.add(keys::constraints::max, vmax);
  return *a;
}

} // namespace meta::presets
