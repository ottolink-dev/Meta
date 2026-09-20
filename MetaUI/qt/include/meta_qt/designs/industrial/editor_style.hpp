/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include "meta_qt/ui/theme.hpp"
#include <QAbstractButton>
#include <QLabel>
#include <QLayout>
#include <QPalette>
namespace meta::qt::industrial
{
inline void style_editor(QWidget *host, const Theme &theme)
{
  host->setObjectName("IndustrialEditor");
  QPalette palette = host->palette();
  palette.setColor(QPalette::Base, theme.bar);
  palette.setColor(QPalette::Window, theme.section_surface);
  palette.setColor(QPalette::Text, theme.ink_primary);
  palette.setColor(QPalette::WindowText, theme.ink_primary);
  palette.setColor(QPalette::ButtonText, theme.ink_primary);
  palette.setColor(QPalette::Button, theme.field);
  palette.setColor(QPalette::Mid, theme.field_border);
  palette.setColor(QPalette::Dark, theme.hairline);
  palette.setColor(QPalette::Light, theme.thumb_top);
  palette.setColor(QPalette::Highlight, theme.accent);
  palette.setColor(QPalette::PlaceholderText, theme.ink_secondary);
  for (auto role : {QPalette::Text, QPalette::WindowText, QPalette::ButtonText})
    palette.setColor(QPalette::Disabled, role, theme.ink_dim);
  host->setPalette(palette);
  host->setFont(row_label_font());
  host->setStyleSheet(
      QString(
          // Plain containers, or a host `*` background rule paints a slab
          "#IndustrialEditor .QWidget { background: transparent; }"
          "#IndustrialEditor QLabel { color: %1; background: transparent; }"
          "#IndustrialEditor QPushButton, #IndustrialEditor QToolButton, "
          "#IndustrialEditor QComboBox {"
          " color: %1; background: %2; border: 1px solid %3; border-radius: "
          "5px; "
          "padding: 3px 7px; }"
          "#IndustrialEditor QPushButton:hover, #IndustrialEditor "
          "QToolButton:hover { "
          "border-color: %4; }"
          "#IndustrialEditor QPushButton:checked { background: %4; color: %1; }"
          "#IndustrialEditor QPushButton:disabled, #IndustrialEditor "
          "QToolButton:disabled { color: %5; background: transparent; "
          "border-color: %3; }"
          "#IndustrialEditor QScrollArea { border: none; background: "
          "transparent; }"
          "#IndustrialEditor QScrollBar:vertical { width: 8px; background: "
          "transparent; }"
          "#IndustrialEditor QScrollBar::handle:vertical { background: %3; "
          "border-radius: 3px; min-height: 24px; }"
          "#IndustrialEditor QScrollBar::add-line:vertical, #IndustrialEditor "
          "QScrollBar::sub-line:vertical { height: 0px; }"
          "#IndustrialEditor QScrollBar::add-page:vertical, #IndustrialEditor "
          "QScrollBar::sub-page:vertical { background: transparent; }"
          "#IndustrialEditor QComboBox::drop-down { border: none; width: 20px; "
          "}"
          "#IndustrialEditor QToolButton::menu-indicator { image: none; }"
          "#IndustrialEditor QPushButton[preset_name] { padding: 2px; border: "
          "2px solid "
          "transparent; border-radius: 5px; background: transparent; }"
          "#IndustrialEditor QPushButton[preset_name]:hover { border-color: "
          "%5; }"
          "#IndustrialEditor QPushButton[preset_name]:checked { border-color: "
          "%4; "
          "background: transparent; }")
          .arg(theme.ink_primary.name(),
               theme.field.name(),
               theme.field_border.name(),
               theme.accent.name(),
               theme.ink_dim.name()));
  if (host->layout()) host->layout()->setSpacing(8);
  for (auto *child : host->findChildren<QWidget *>())
  {
    child->setProperty("industrialEditor", true);
    child->setPalette(palette);
  }
  for (auto *label : host->findChildren<QLabel *>())
  {
    label->setFont(row_label_font());
    auto text = label->text();
    if (!text.isEmpty())
    {
      text[0] = text[0].toUpper();
      label->setText(text);
    }
  }
  for (auto *button : host->findChildren<QAbstractButton *>())
  {
    if (button->property("preset_name").isValid()) continue;
    button->setFont(ui_font(12));
    button->setFixedHeight(28);
    button->setCursor(Qt::PointingHandCursor);
  }
}
} // namespace meta::qt::industrial
