/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include "meta/core/attribute.hpp"

#ifdef META_ENABLE_GLM_TYPES
#include <glm/glm.hpp>
#include <vector>

namespace meta::presets
{

Attribute<glm::vec2> &wavenumber(AttributeContainer &c,
                                 std::string_view    key,
                                 std::string_view    label,
                                 glm::vec2           value,
                                 float               vmin,
                                 float               vmax,
                                 bool                link_xy,
                                 std::string_view    format = "{:.2f}");

Attribute<glm::vec2> &range(AttributeContainer &c,
                            std::string_view    key,
                            std::string_view    label,
                            glm::vec2           value,
                            float               vmin,
                            float               vmax,
                            bool                is_active,
                            std::string_view    format = "{:.3f}");

Attribute<glm::vec2> &xy(AttributeContainer &c,
                         std::string_view    key,
                         std::string_view    label,
                         glm::vec2           value,
                         float               xmin,
                         float               xmax,
                         float               ymin,
                         float               ymax);

Attribute<std::vector<glm::vec3>> &points(AttributeContainer    &c,
                                          std::string_view       key,
                                          std::string_view       label,
                                          std::vector<glm::vec3> value = {});

Attribute<glm::vec4> &color(AttributeContainer &c,
                            std::string_view    key,
                            std::string_view    label,
                            glm::vec4           value);

} // namespace meta::presets
#endif // META_ENABLE_GLM_TYPES
