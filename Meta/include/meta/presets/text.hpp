/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include "meta/core/attribute.hpp"
#include <string>

namespace meta::presets
{

Attribute<std::string> &text(AttributeContainer &c,
                             std::string_view    key,
                             std::string_view    label,
                             std::string         value,
                             bool                read_only = false);

} // namespace meta::presets
