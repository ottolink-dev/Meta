/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include "meta/core/attribute.hpp"

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
#include "meta/ext/color_gradient/color_gradient.hpp"

namespace meta::presets
{

Attribute<ColorGradient> &color_gradient(AttributeContainer &c,
                                         std::string_view    key,
                                         std::string_view    label,
                                         ColorGradient       value = {});

} // namespace meta::presets
#endif // META_ENABLE_COLOR_GRADIENT_TYPES
