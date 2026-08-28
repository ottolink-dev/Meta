/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <string>

namespace meta::qt::industrial
{

/// Design name to pass to render_row() / DesignRegistry.
inline constexpr char kDesignName[] = "industrial";

/** @brief Register every industrial control with the DesignRegistry.
 *
 * Idempotent, so a host may call it unconditionally at startup.
 *
 * Only the widget types this design actually implements are registered.
 * Everything else resolves to nothing and falls back to the stock renderer,
 * which is what keeps a partial design a usable panel rather than a broken one.
 *
 * Note there is deliberately no "stock" design to register: an unknown design
 * name finds no factories and every row falls back, so `design = "stock"` gives
 * the unmodified Qt look for free. That makes A/B comparison a settings change
 * rather than a build.
 */
void register_design();

} // namespace meta::qt::industrial
