/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once

namespace meta::qt::stock
{

/// Design name to pass to render_row() / DesignRegistry.
inline constexpr char kDesignName[] = "stock";

/** @brief Register the built-in widgets as a design.
 *
 * Idempotent.
 *
 * "stock" is a flavour like any other, at the same level as "industrial" --
 * not a privileged fallback baked into the dispatcher. Each entry is a thin
 * wrapper around the existing WidgetRenderer<T>, registered under the wildcard
 * widget_type because WidgetRenderer<T> already resolves widget_type itself.
 * No stock widget is rewritten and WidgetRenderer<T> stays a public
 * compile-time extension point.
 *
 * Registering these is what lets DesignRegistry be the only dispatch: with
 * stock present as data, render_row() no longer needs a hardcoded call to
 * qt::render(), and the typeid if-chain behind it becomes dead weight.
 */
void register_design();

} // namespace meta::qt::stock
