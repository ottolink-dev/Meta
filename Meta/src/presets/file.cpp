/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/presets/file.hpp"
#include "meta/core/attribute_container.hpp"
#include "meta/metadata/keys.hpp"

namespace meta::presets
{

Attribute<std::filesystem::path> &file(AttributeContainer &c, std::string_view key,
                                       std::string_view label, std::filesystem::path value,
                                       std::string_view filter, bool for_saving)
{
  auto *a = c.add(std::string(key), std::move(value));
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, for_saving ? "SaveFile" : "OpenFile");
  m.add(keys::ui::label, std::string(label));
  m.add(keys::constraints::file_filter, std::string(filter));
  return *a;
}

} // namespace meta::presets
