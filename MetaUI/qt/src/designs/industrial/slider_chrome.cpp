/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/slider_chrome.hpp"

#include <algorithm>
#include <cmath>

#include <QLinearGradient>
#include <QPainter>

namespace meta::qt::industrial
{

SliderGeometry SliderGeometry::compute(const Theme &theme,
                                       int          width,
                                       int          height,
                                       qreal        norm)
{
  const Metrics &m = theme.metrics;

  SliderGeometry g;

  const int label_width = int(std::clamp<qreal>(width * m.label_width_ratio,
                                                m.label_min_width,
                                                m.label_max_width));

  // The narrow branch keys off this row's own width, not the window's.
  const int field_width = width < m.narrow_threshold ? m.value_field_width_narrow
                                                     : m.value_field_width;

  g.label = QRect(0, 0, label_width, height);

  const int x0 = label_width + m.gap;
  const int x1 = width - field_width - m.gap;

  g.rail = QRect(x0, (height - m.rail_height) / 2, std::max(0, x1 - x0), m.rail_height);

  const int travel = std::max(0, g.rail.width() - m.thumb_width);
  g.thumb = QRect(g.rail.x() + int(std::round(std::clamp(norm, 0.0, 1.0) * travel)),
                  (height - m.thumb_height) / 2,
                  m.thumb_width,
                  m.thumb_height);

  g.fill = QRect(g.rail.x(),
                 g.rail.y(),
                 std::clamp(g.thumb.center().x() - g.rail.x(), 0, g.rail.width()),
                 g.rail.height());

  g.field = QRect(width - field_width,
                  (height - m.value_field_height) / 2,
                  field_width,
                  m.value_field_height);

  return g;
}

void paint_slider_row(QPainter             &painter,
                      const Theme          &theme,
                      const SliderGeometry &geometry,
                      const SliderVisual   &visual,
                      int)
{
  painter.setRenderHint(QPainter::Antialiasing, true);

  const Metrics &m = theme.metrics;

  // --- label. Text is the only thing state is allowed to change.
  QFont label_font = ui_font(12, false, 1.0);
  label_font.setCapitalization(QFont::AllUppercase);
  painter.setFont(label_font);
  painter.setPen(theme.state_ink(visual.modified, visual.locked));
  painter.drawText(geometry.label, Qt::AlignLeft | Qt::AlignVCenter, visual.label);

  if (geometry.rail.width() <= 0) return;

  // --- rail well
  painter.setPen(QPen(theme.rail_well_border, 1));
  painter.setBrush(theme.rail_well);
  painter.drawRoundedRect(QRectF(geometry.rail).adjusted(0.5, 0.5, -0.5, -0.5),
                          m.rail_radius,
                          m.rail_radius);

  // --- fill. Always the group accent; never a state colour.
  if (geometry.fill.width() > 0)
  {
    painter.setPen(Qt::NoPen);
    painter.setBrush(theme.rail_fill(visual.category, visual.locked));
    painter.drawRoundedRect(QRectF(geometry.fill).adjusted(0.5, 0.5, -0.5, -0.5),
                            m.rail_radius,
                            m.rail_radius);
  }

  // --- thumb
  painter.setOpacity(visual.locked ? theme.locked_thumb_alpha : 1.0);

  QLinearGradient metal(geometry.thumb.topLeft(), geometry.thumb.bottomLeft());
  metal.setColorAt(0.0, theme.thumb_top);
  metal.setColorAt(1.0, theme.thumb_bottom);

  painter.setPen(QPen(theme.thumb_border, 1));
  painter.setBrush(metal);
  painter.drawRoundedRect(QRectF(geometry.thumb).adjusted(0.5, 0.5, -0.5, -0.5),
                          m.radius,
                          m.radius);

  // grip notch, 2x8 centred
  painter.setPen(Qt::NoPen);
  painter.setBrush(theme.thumb_grip);
  painter.drawRect(
      QRect(geometry.thumb.center().x(), geometry.thumb.center().y() - 3, 2, 8));

  painter.setOpacity(1.0);
}

QString field_stylesheet(const Theme &theme, bool editing, bool modified, bool locked)
{
  const QColor bg = editing ? theme.field_editing : theme.field;
  const QColor border = editing ? theme.accent : theme.field_border;

  return QString("QLineEdit {"
                 " background: %1;"
                 " border: 1px solid %2;"
                 " border-radius: %3px;"
                 " color: %4;"
                 " padding-right: 4px;"
                 "}")
      .arg(bg.name())
      .arg(border.name())
      .arg(theme.metrics.radius)
      .arg(theme.state_ink(modified, locked).name());
}

} // namespace meta::qt::industrial
