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
 * Only the widget types this design actually implements are registered. It also
 * registers the stock design and declares stock as its fallback, so anything
 * not yet ported still renders -- a partial design stays a usable panel rather
 * than a handful of rows with gaps between them.
 *
 * "stock" is a peer flavour, not a privileged fallback: selecting it directly
 * gives the unmodified Qt look, which makes A/B comparison a settings change
 * rather than a rebuild.
 */
void register_design();

} // namespace meta::qt::industrial
