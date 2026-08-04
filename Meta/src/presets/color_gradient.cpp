/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/presets/color_gradient.hpp"

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
#include "meta/core/attribute_container.hpp"
#include "meta/metadata/keys.hpp"

namespace meta::presets
{

Attribute<ColorGradient> &color_gradient(AttributeContainer &c, std::string_view key,
                                         std::string_view label, ColorGradient value)
{
  auto *a = c.add(std::string(key), std::move(value));
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "GradientEditor");
  m.add(keys::ui::label, std::string(label));
  return *a;
}

} // namespace meta::presets
#endif // META_ENABLE_COLOR_GRADIENT_TYPES
