/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include "meta_qt/container_widget.hpp"
#include "meta_qt/ui/theme.hpp"

namespace meta::qt::industrial
{

/** @brief Section factory matching the industrial rows.
 *
 * Returns a stock CollapsibleSection wearing a theme-derived stylesheet rather
 * than a subclass. The header is a QToolButton, and a stylesheet can restyle it
 * completely without reaching into CollapsibleSection's internals, so the
 * design gets its look without Meta having to widen that class's interface.
 *
 * `theme` must outlive every section the factory builds. Pass one owned by the
 * ThemeRegistry.
 */
SectionFactory make_section_factory(const Theme &theme);

} // namespace meta::qt::industrial
