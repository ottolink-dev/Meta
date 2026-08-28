/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/check_row.hpp"

#include <cmath>

#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>

namespace meta::qt::industrial
{

CheckRow::CheckRow(Attribute<bool> &attr, const RowContext &ctx, QWidget *parent)
    : Control<bool>(ctx, parent)
{
  key_ = attr.name();
  label_ = meta::common::label(attr);
  value_ = attr.value();
  knob_ = value_ ? 1.0 : 0.0;

  setFixedHeight(theme().metrics.check_row_height);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  glide_ = new Glide(theme().metrics.switch_ms, this);
  connect(glide_,
          &Glide::tick,
          this,
          [this](qreal t)
          {
            knob_ = t;
            update();
          });
  glide_->jump(knob_);
}

void CheckRow::set(const bool &value)
{
  if (value_ == value) return;
  value_ = value;
  glide_->to(value_ ? 1.0 : 0.0);
  update();
}

QSize CheckRow::sizeHint() const
{
  const Metrics &m = theme().metrics;
  return QSize(m.label_min_width + m.gap + m.switch_width, m.check_row_height);
}

QRect CheckRow::switch_rect() const
{
  const Metrics &m = theme().metrics;
  return QRect(width() - m.switch_width,
               (height() - m.switch_height) / 2,
               m.switch_width,
               m.switch_height);
}

void CheckRow::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const Theme   &t = theme();
  const Metrics &m = t.metrics;
  const bool     locked = is_locked();

  // --- label
  QFont label_font = ui_font(12, false, 1.0);
  label_font.setCapitalization(QFont::AllUppercase);
  painter.setFont(label_font);
  painter.setPen(t.state_ink(is_modified(), locked));
  const int label_w = width() - m.switch_width - m.gap;
  painter.drawText(QRect(0, 0, label_w, height()),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   elide_label(QString::fromStdString(label_), label_font, label_w));

  painter.setOpacity(locked ? t.locked_thumb_alpha : 1.0);

  // --- track. On uses the accent, consistent with the rail: fills are accent,
  // never a state colour.
  const QRect track = switch_rect();
  QColor      track_color = t.switch_track_off;
  if (knob_ > 0.0)
  {
    QColor on = t.switch_track_on();
    // blend across the travel so the track follows the knob rather than
    // snapping at the midpoint
    track_color = QColor::fromRgbF(
        t.switch_track_off.redF() * (1.0 - knob_) + on.redF() * knob_,
        t.switch_track_off.greenF() * (1.0 - knob_) + on.greenF() * knob_,
        t.switch_track_off.blueF() * (1.0 - knob_) + on.blueF() * knob_);
  }

  painter.setPen(QPen(t.hairline, 1));
  painter.setBrush(track_color);
  painter.drawRoundedRect(QRectF(track).adjusted(0.5, 0.5, -0.5, -0.5),
                          m.radius,
                          m.radius);

  // --- knob
  const int travel = m.switch_width - m.knob_size - 2 * m.knob_inset;
  const QRect knob(track.x() + m.knob_inset + int(std::round(knob_ * travel)),
                   track.y() + m.knob_inset,
                   m.knob_size,
                   m.switch_height - 2 * m.knob_inset);

  QLinearGradient metal(knob.topLeft(), knob.bottomLeft());
  metal.setColorAt(0.0, value_ ? t.knob_on_top : t.knob_off_top);
  metal.setColorAt(1.0, value_ ? t.knob_on_bottom : t.knob_off_bottom);

  painter.setPen(QPen(t.thumb_border, 1));
  painter.setBrush(metal);
  painter.drawRoundedRect(QRectF(knob).adjusted(0.5, 0.5, -0.5, -0.5),
                          m.radius,
                          m.radius);

  painter.setOpacity(1.0);
}

void CheckRow::mousePressEvent(QMouseEvent *event)
{
  if (is_locked() || event->button() != Qt::LeftButton)
  {
    event->ignore();
    return;
  }

  setFocus(Qt::MouseFocusReason);
  pressed_ = true;
}

void CheckRow::mouseReleaseEvent(QMouseEvent *event)
{
  if (!pressed_) return;
  pressed_ = false;

  // The whole row is the hit target, not just the switch.
  if (rect().contains(event->pos())) toggle();
}

void CheckRow::keyPressEvent(QKeyEvent *event)
{
  if (!is_locked() && (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return))
  {
    toggle();
    event->accept();
    return;
  }

  Control<bool>::keyPressEvent(event);
}

void CheckRow::toggle()
{
  value_ = !value_;
  glide_->to(value_ ? 1.0 : 0.0);
  update();

  // A discrete control has no drag, so the whole edit is one instant: the
  // binder still sees a well-formed started/changed/ended sequence.
  begin_edit();
  notify_value_changed();
  end_edit();
}

} // namespace meta::qt::industrial
