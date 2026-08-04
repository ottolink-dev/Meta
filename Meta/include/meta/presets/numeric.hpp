/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include "meta/core/attribute.hpp"

namespace meta::presets
{

Attribute<float> &angle(AttributeContainer &c,
                        std::string_view    key,
                        std::string_view    label,
                        float               value = 0);

Attribute<int> &seed(AttributeContainer &c,
                     std::string_view    key,
                     std::string_view    label,
                     int                 value = 0);

Attribute<float> &slider_float(AttributeContainer &c,
                               std::string_view    key,
                               std::string_view    label,
                               float               value,
                               float               vmin,
                               float               vmax,
                               std::string_view    format = "{:.3f}",
                               bool                log_scale = false);

Attribute<int> &slider_int(AttributeContainer &c,
                           std::string_view    key,
                           std::string_view    label,
                           int                 value,
                           int                 vmin,
                           int                 vmax,
                           std::string_view    format = "{}");

} // namespace meta::presets