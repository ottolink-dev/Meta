/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "meta/core/attribute.hpp"

namespace meta::presets
{

Attribute<bool> &checkbox(AttributeContainer &c,
                          std::string_view    key,
                          std::string_view    label,
                          bool                value);

Attribute<bool> &toggle_button(AttributeContainer &c,
                               std::string_view    key,
                               std::string_view    label,
                               bool                value);

Attribute<bool> &binary_buttons(AttributeContainer &c,
                                std::string_view    key,
                                std::string_view    label,
                                std::string_view    label_true,
                                std::string_view    label_false,
                                bool                value);

Attribute<int> &enum_choice(
    AttributeContainer                             &c,
    std::string_view                                key,
    std::string_view                                label,
    const std::vector<std::pair<int, std::string>> &items,
    int                                             value);

Attribute<std::string> &string_choice(AttributeContainer             &c,
                                      std::string_view                key,
                                      std::string_view                label,
                                      const std::vector<std::string> &choices,
                                      std::string                     value,
                                      bool use_combo = true);

} // namespace meta::presets
