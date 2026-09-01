/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/section.hpp"

#include "meta_qt/widgets/collapsible_section.hpp"

namespace meta::qt::industrial
{

SectionFactory make_section_factory(const Theme &theme)
{
  return [&theme](const QString &title) -> CollapsibleSection *
  {
    auto *section = new CollapsibleSection(title);

    const Metrics &m = theme.metrics;

    // The header is the section's QToolButton. Styling it by type selector
    // avoids depending on any private member of CollapsibleSection.
    //
    // The stock header renders as a checked QToolButton, which most styles
    // paint in the platform highlight colour. That is why an unstyled panel
    // shows blue bars between grey rows.
    const QString style =
        QString("QToolButton {"
                " background: %1;"
                " color: %2;"
                " border: none;"
                " border-top: 1px solid %3;"
                " border-bottom: 1px solid %4;"
                " border-radius: %5px;"
                " padding: 0px 8px;"
                " min-height: %6px;"
                " text-align: left;"
                " font-weight: bold;"
                "}"
                "QToolButton:hover { background: %7; }"
                "QToolButton:pressed { background: %8; }"
                "QToolButton:checked { background: %1; color: %2; }")
            .arg(theme.section_header.name())
            .arg(theme.ink_section_title.name())
            .arg(theme.bevel_top.name())
            .arg(theme.bevel_bottom.name())
            .arg(m.radius)
            .arg(m.section_header_height)
            .arg(theme.section_header_hover.name())
            .arg(theme.section_header_press.name());

    section->setStyleSheet(style);

    if (section->content_layout)
    {
      section->content_layout->setContentsMargins(m.section_body_padding_x,
                                                  m.section_body_padding_y,
                                                  m.section_body_padding_x,
                                                  m.section_body_padding_y);
      section->content_layout->setSpacing(m.section_row_spacing);
    }

    return section;
  };
}

} // namespace meta::qt::industrial
