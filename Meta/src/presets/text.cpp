/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/presets/text.hpp"
#include "meta/core/attribute_container.hpp"
#include "meta/metadata/keys.hpp"

namespace meta::presets
{

Attribute<std::string> &text(AttributeContainer &c,
                             std::string_view    key,
                             std::string_view    label,
                             std::string         value,
                             bool                read_only)
{
  auto *a = c.add(std::string(key), std::move(value));
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "SingleLineText");
  m.add(keys::ui::label, std::string(label));
  if (read_only) m.add(keys::ui::read_only, true);
  return *a;
}

} // namespace meta::presets
