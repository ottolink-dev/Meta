/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include "meta/core/attribute.hpp"
#include <filesystem>

namespace meta::presets
{

Attribute<std::filesystem::path> &file(AttributeContainer   &c,
                                       std::string_view      key,
                                       std::string_view      label,
                                       std::filesystem::path value,
                                       std::string_view      filter,
                                       bool                  for_saving);

} // namespace meta::presets
