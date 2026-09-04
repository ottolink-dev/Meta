/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <QString>

#include "meta_qt/ui/theme.hpp"

namespace meta::qt::industrial
{

/** @brief Scrollbar styling for the panel's scroll area.
 *
 * Thin, no arrows, handle only. The scrollbar should be reserved permanently by
 * the caller (ScrollBarAlwaysOn): an as-needed bar appearing on expand narrows
 * the viewport and reflows every row.
 */
QString scrollbar_stylesheet(const Theme &theme);

/** @brief Tooltip styling.
 *
 * Applied application-wide by whoever owns the app, because QToolTip is styled
 * globally rather than per-widget.
 *
 * Note this cannot animate. QToolTip is a static utility with no widget to
 * attach an animation to; fading requires replacing it with a custom popup.
 */
QString tooltip_stylesheet(const Theme &theme);

} // namespace meta::qt::industrial
