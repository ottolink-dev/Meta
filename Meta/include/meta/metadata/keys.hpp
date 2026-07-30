/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once

namespace meta::keys::constraints
{

inline constexpr char allowed_values[] = "constraints.allowed_values";
inline constexpr char aspect_ratio[] = "constraints.aspect_ratio";
inline constexpr char enum_items[] = "constraints.enum_items";
inline constexpr char file_filter[] = "constraints.file_filter";
inline constexpr char min[] = "constraints.min";
inline constexpr char max[] = "constraints.max";
inline constexpr char power_of_two[] = "constraints.power_of_two";
inline constexpr char step[] = "constraints.step";

} // namespace meta::keys::constraints

namespace meta::keys::ui
{

inline constexpr char format[] = "ui.format";
inline constexpr char category[] = "ui.category";
inline constexpr char label[] = "ui.label";
inline constexpr char label_true[] = "ui.label_true";
inline constexpr char label_false[] = "ui.label_false";
inline constexpr char state[] = "ui.state";
inline constexpr char widget_type[] = "ui.widget_type";
inline constexpr char data_provider[] = "ui.data_provider";
inline constexpr char tooltip[]       = "ui.tooltip";
inline constexpr char presets[]       = "ui.presets";

// Ad-hoc keys hoisted from raw string literals (Phase C migration). Each value
// is byte-identical to the literal it replaced; producers set them and
// renderers/parity consume them across files, so a typo must be a compile error.
inline constexpr char active[]            = "ui.active";
inline constexpr char has_active_toggle[] = "ui.has_active_toggle";
inline constexpr char locked_xy[]         = "ui.locked_xy";
inline constexpr char min_x[]             = "ui.min_x";
inline constexpr char max_x[]             = "ui.max_x";
inline constexpr char min_y[]             = "ui.min_y";
inline constexpr char max_y[]             = "ui.max_y";
inline constexpr char log_scale[]         = "ui.log_scale";
inline constexpr char read_only[]         = "ui.read_only";
inline constexpr char closed[]            = "ui.closed";

} // namespace meta::keys::ui