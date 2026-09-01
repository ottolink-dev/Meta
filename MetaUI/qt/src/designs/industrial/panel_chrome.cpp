/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/panel_chrome.hpp"

namespace meta::qt::industrial
{

QString scrollbar_stylesheet(const Theme &theme)
{
  return QString("QScrollArea { background: transparent; border: none; }"
                 "QScrollBar:vertical {"
                 " background: transparent;"
                 " width: 10px;"
                 " margin: 0px;"
                 "}"
                 "QScrollBar::handle:vertical {"
                 " background: %1;"
                 " min-height: 30px;"
                 " border-radius: 2px;"
                 " margin: 2px 3px 2px 3px;"
                 "}"
                 "QScrollBar::handle:vertical:hover { background: %2; }"
                 // No arrows, and no page-step background: the groove should be
                 // invisible so only the handle reads as chrome.
                 "QScrollBar::add-line:vertical,"
                 "QScrollBar::sub-line:vertical { height: 0px; }"
                 "QScrollBar::add-page:vertical,"
                 "QScrollBar::sub-page:vertical { background: transparent; }")
      .arg(theme.field_border.name())
      .arg(theme.field_border_hover.name());
}

QString tooltip_stylesheet(const Theme &theme)
{
  return QString("QToolTip {"
                 " background: %1;"
                 " color: %2;"
                 " border: 1px solid %3;"
                 " border-radius: %4px;"
                 " padding: 4px 7px;"
                 " font-size: 11px;"
                 "}")
      .arg(theme.bar.name())
      .arg(theme.ink_primary.name())
      .arg(theme.hairline.name())
      .arg(theme.metrics.radius);
}

} // namespace meta::qt::industrial
