/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/presets/glm.hpp"

#ifdef META_ENABLE_GLM_TYPES
#include "meta/core/attribute_container.hpp"
#include "meta/metadata/keys.hpp"

namespace meta::presets
{

Attribute<glm::vec2> &wavenumber(AttributeContainer &c,
                                 std::string_view    key,
                                 std::string_view    label,
                                 glm::vec2           value,
                                 float               vmin,
                                 float               vmax,
                                 bool                link_xy,
                                 std::string_view    format)
{
  auto *a = c.add(std::string(key), value);
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "LinkedSliders");
  m.add(keys::ui::label, std::string(label));
  m.add(keys::ui::format, std::string(format));
  a->state().add(keys::state::locked_xy, link_xy);
  m.add(keys::constraints::min, vmin);
  m.add(keys::constraints::max, vmax);
  return *a;
}

Attribute<glm::vec2> &range(AttributeContainer &c,
                            std::string_view    key,
                            std::string_view    label,
                            glm::vec2           value,
                            float               vmin,
                            float               vmax,
                            bool                is_active,
                            std::string_view    format)
{
  auto *a = c.add(std::string(key), value);
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "RangeBar");
  m.add(keys::ui::label, std::string(label));
  m.add(keys::ui::format, std::string(format));
  m.add(keys::constraints::min, vmin);
  m.add(keys::constraints::max, vmax);
  a->state().add(keys::state::active, is_active);
  return *a;
}

Attribute<glm::vec2> &xy(AttributeContainer &c,
                         std::string_view    key,
                         std::string_view    label,
                         glm::vec2           value,
                         float               xmin,
                         float               xmax,
                         float               ymin,
                         float               ymax)
{
  auto *a = c.add(std::string(key), value);
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "XYCanvas");
  m.add(keys::ui::label, std::string(label));
  m.add(keys::constraints::min, xmin);
  m.add(keys::constraints::max, xmax);
  m.add(keys::ui::min_x, xmin);
  m.add(keys::ui::max_x, xmax);
  m.add(keys::ui::min_y, ymin);
  m.add(keys::ui::max_y, ymax);
  return *a;
}

Attribute<std::vector<glm::vec3>> &points(AttributeContainer    &c,
                                          std::string_view       key,
                                          std::string_view       label,
                                          std::vector<glm::vec3> value)
{
  auto *a = c.add(std::string(key), std::move(value));
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "PointsEditor");
  m.add(keys::ui::label, std::string(label));
  return *a;
}

Attribute<glm::vec4> &color(AttributeContainer &c,
                            std::string_view    key,
                            std::string_view    label,
                            glm::vec4           value)
{
  auto *a = c.add(std::string(key), value);
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "ColorPicker");
  m.add(keys::ui::label, std::string(label));
  return *a;
}

} // namespace meta::presets
#endif // META_ENABLE_GLM_TYPES
