/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

#include <algorithm>
#include <cmath>
#include <format>
#include <random>

#include <QApplication>
#include <QFontMetrics>
#include <QHoverEvent>
#include <QMenu>
#include <QPainter>

#include "meta/logger.hpp"

#include "meta_qt/widgets/helpers.hpp"
#include "meta_qt/widgets/slider_float.hpp"

namespace meta::qt
{

namespace
{
// Smallest value considered by the log mapping, avoids log10(0) / negative
// domain issues when vmin is 0 or slightly negative due to float error.
constexpr float k_log_epsilon = 1e-6f;
} // namespace

SliderFloat::SliderFloat(const std::string &label_,
                         float              value_init_,
                         float              vmin_,
                         float              vmax_,
                         bool               add_plus_minus_buttons_,
                         const std::string &value_format_,
                         bool               log_scale_,
                         QWidget           *parent)
    : QWidget(parent),
      value_init(value_init_),
      value(value_init_),
      vmin(vmin_),
      vmax(vmax_),
      add_plus_minus_buttons(add_plus_minus_buttons_),
      value_format(value_format_),
      log_scale(log_scale_)
{
  this->label = helpers::truncate_string(label_, this->style.label_max_len());

  this->setMouseTracking(true);
  this->setAttribute(Qt::WA_Hover);
  this->setContextMenuPolicy(Qt::CustomContextMenu);

  this->update_geometry();
  this->connect(this,
                &SliderFloat::edit_ended,
                [this]() { this->update_geometry(); });

  // Derive stylesheet colours from the current palette so we respect dark/light
  // themes.
  const QPalette &pal = this->palette();
  this->style_sheet = "background-color: " +
                      pal.color(QPalette::Base).name().toStdString() +
                      "; color: " +
                      pal.color(QPalette::Text).name().toStdString() +
                      "; border: 0px;"
                      " selection-background-color: #ABABAB;";

  this->value_edit = new QLineEdit(this);
  this->value_edit->setVisible(false);
  this->value_edit->setFixedHeight(this->height() - 2);
  this->value_edit->setAlignment(Qt::AlignCenter);
  this->value_edit->setStyleSheet(this->style_sheet.c_str());
  this->connect(this->value_edit,
                &QLineEdit::editingFinished,
                this,
                &SliderFloat::apply_text_edit_value);
}

void SliderFloat::apply_text_edit_value()
{
  bool  ok = false;
  float new_value = this->value_edit->text().toFloat(&ok);
  if (ok && this->set_value(new_value)) Q_EMIT this->edit_ended();

  this->value_edit->setVisible(false);
  this->update();
}

bool SliderFloat::event(QEvent *event)
{
  switch (event->type())
  {
  case QEvent::HoverEnter:
    this->is_hovered = true;
    this->update();
    this->setCursor(Qt::SizeHorCursor);
    break;

  case QEvent::HoverLeave:
    this->is_hovered = false;
    this->is_minus_hovered = false;
    this->is_plus_hovered = false;
    this->is_bar_hovered = false;
    this->update();
    this->setCursor(Qt::ArrowCursor);
    break;

  case QEvent::HoverMove:
  {
    auto        *hover = static_cast<QHoverEvent *>(event);
    const QPoint pos = hover->position().toPoint();
    this->is_minus_hovered = this->rect_minus.contains(pos);
    this->is_plus_hovered = this->rect_plus.contains(pos);
    this->is_bar_hovered = this->rect_bar.contains(pos);
    this->update();
    this->setCursor(this->is_bar_hovered ? Qt::SizeHorCursor : Qt::ArrowCursor);
    break;
  }

  default: break;
  }
  return QWidget::event(event);
}

float SliderFloat::get_value() const { return this->value; }

std::string SliderFloat::get_value_as_string() const
{
  return std::vformat(this->value_format, std::make_format_args(this->value));
}

// ---------------------------------------------------------------------------
// Log-scale mapping helpers
// ---------------------------------------------------------------------------

float SliderFloat::value_to_ratio(float v) const
{
  if (!this->log_scale)
  {
    const float range = this->vmax - this->vmin;
    return range > 0.f ? std::clamp((v - this->vmin) / range, 0.f, 1.f) : 0.f;
  }

  const float lmin = std::log10(std::max(this->vmin, k_log_epsilon));
  const float lmax = std::log10(std::max(this->vmax, k_log_epsilon));
  const float lv = std::log10(std::max(v, k_log_epsilon));
  const float range = lmax - lmin;

  return range > 0.f ? std::clamp((lv - lmin) / range, 0.f, 1.f) : 0.f;
}

// ---------------------------------------------------------------------------
// Bounded vs unbounded
// ---------------------------------------------------------------------------

bool SliderFloat::is_range_bounded() const
{
  return this->vmin != -FLT_MAX && this->vmax != FLT_MAX &&
         this->vmax > this->vmin;
}

// Handle of an unbounded slider: centred at rest, following the drag while one
// is in progress, clamped so it never leaves the track. Its travel is only an
// affordance - the value keeps changing once the handle hits an end.
QRect SliderFloat::handle_rect() const
{
  const int track_w = std::max(this->rect_bar.width() - 2, 2);
  const int handle_w = std::clamp(3 * this->base_dx, 2, track_w);
  const int span = (track_w - handle_w) / 2;

  QRect r = this->rect_bar.adjusted(1, 1, -1, -1);
  r.setWidth(handle_w);
  r.moveLeft(this->rect_bar.left() + 1 + span +
             std::clamp(this->drag_dx, -span, span));

  return r;
}

float SliderFloat::ratio_to_value(float r) const
{
  r = std::clamp(r, 0.f, 1.f);

  if (!this->log_scale) return this->vmin + r * (this->vmax - this->vmin);

  const float lmin = std::log10(std::max(this->vmin, k_log_epsilon));
  const float lmax = std::log10(std::max(this->vmax, k_log_epsilon));
  const float lv = lmin + r * (lmax - lmin);

  return std::pow(10.f, lv);
}

void SliderFloat::mouseDoubleClickEvent(QMouseEvent *)
{
  const bool is_bounded = this->is_range_bounded();

  if (this->is_bar_hovered)
  {
    this->value_edit->setText(QString::number(this->value));
    this->value_edit->setGeometry(this->rect_bar.adjusted(1, 1, -1, -1));
    this->value_edit->setVisible(true);
    this->value_edit->setFocus();
    this->value_edit->selectAll();
    this->update();
  }
  else if (this->is_minus_hovered)
  {
    if (this->log_scale)
    {
      const float r = this->value_to_ratio(this->value);
      const float dr = 1.f / float(this->style.button_ticks());
      if (this->set_value(this->ratio_to_value(r - dr)))
        Q_EMIT this->edit_ended();
    }
    else
    {
      const float delta = is_bounded ? (this->vmax - this->vmin) /
                                           float(this->style.button_ticks())
                                     : 1.f;
      if (this->set_value(this->value - delta)) Q_EMIT this->edit_ended();
    }
  }
  else if (this->is_plus_hovered)
  {
    if (this->log_scale)
    {
      const float r = this->value_to_ratio(this->value);
      const float dr = 1.f / float(this->style.button_ticks());
      if (this->set_value(this->ratio_to_value(r + dr)))
        Q_EMIT this->edit_ended();
    }
    else
    {
      const float delta = is_bounded ? (this->vmax - this->vmin) /
                                           float(this->style.button_ticks())
                                     : 1.f;
      if (this->set_value(this->value + delta)) Q_EMIT this->edit_ended();
    }
  }
}

void SliderFloat::mouseMoveEvent(QMouseEvent *event)
{
  if (!this->is_dragging)
  {
    QWidget::mouseMoveEvent(event);
    return;
  }

  const Qt::KeyboardModifiers mods = event->modifiers();
  this->force_edit_ended_emit = false;

  if (this->log_scale)
  {
    // Drag in ratio space (0..1 across the bar) so pixel-to-value
    // sensitivity is uniform across the whole log range, rather than
    // dominated by the top decade the way a raw-value drag would be.
    float ppu = float(this->rect_bar.width());

    if ((mods & Qt::ControlModifier) && (mods & Qt::ShiftModifier))
      this->force_edit_ended_emit = true;
    else if (mods & Qt::ControlModifier)
      ppu *= PPU_MULT_FINE;
    else if (mods & Qt::ShiftModifier)
      ppu /= PPU_MULT_FINE;

    const int dx = event->position().toPoint().x() -
                   this->pos_x_before_dragging;
    const float dr = float(dx) / ppu;
    const float r_before = this->value_to_ratio(this->value_before_dragging);

    this->drag_dx = dx;
    this->set_value(this->ratio_to_value(r_before + dr));
    this->update(); // the handle moves even when the value is clamped
  }
  else
  {
    float ppu;

    if (!this->is_range_bounded())
      ppu = PPU_F;
    else
      ppu = float(this->rect_bar.width()) / (this->vmax - this->vmin);

    if ((mods & Qt::ControlModifier) && (mods & Qt::ShiftModifier))
      this->force_edit_ended_emit = true;
    else if (mods & Qt::ControlModifier)
      ppu *= PPU_MULT_FINE;
    else if (mods & Qt::ShiftModifier)
      ppu /= PPU_MULT_FINE;

    const int dx = event->position().toPoint().x() -
                   this->pos_x_before_dragging;
    const float dv = float(dx) / ppu;

    this->drag_dx = dx;
    this->set_value(this->value_before_dragging + dv);
    this->update(); // the handle moves even when the value is clamped
  }

  QWidget::mouseMoveEvent(event);
}

void SliderFloat::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    const bool is_bounded = this->is_range_bounded();

    if (this->is_bar_hovered)
    {
      this->value_before_dragging = this->value;
      this->pos_x_before_dragging = event->position().toPoint().x();
      this->set_is_dragging(true);
    }
    else if (this->is_minus_hovered)
    {
      if (this->log_scale)
      {
        const float r = this->value_to_ratio(this->value);
        const float dr = 1.f / float(this->style.button_ticks());
        if (this->set_value(this->ratio_to_value(r - dr)))
          Q_EMIT this->edit_ended();
      }
      else
      {
        const float delta = is_bounded ? (this->vmax - this->vmin) /
                                             float(this->style.button_ticks())
                                       : 1.f;
        if (this->set_value(this->value - delta)) Q_EMIT this->edit_ended();
      }
    }
    else if (this->is_plus_hovered)
    {
      if (this->log_scale)
      {
        const float r = this->value_to_ratio(this->value);
        const float dr = 1.f / float(this->style.button_ticks());
        if (this->set_value(this->ratio_to_value(r + dr)))
          Q_EMIT this->edit_ended();
      }
      else
      {
        const float delta = is_bounded ? (this->vmax - this->vmin) /
                                             float(this->style.button_ticks())
                                       : 1.f;
        if (this->set_value(this->value + delta)) Q_EMIT this->edit_ended();
      }
    }
  }
}

void SliderFloat::mouseReleaseEvent(QMouseEvent *event)
{
  if (this->is_dragging && event->button() == Qt::LeftButton)
  {
    this->set_is_dragging(false);
    if (this->value != this->value_before_dragging) Q_EMIT this->edit_ended();
  }
}

void SliderFloat::paintEvent(QPaintEvent *)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  const QPalette &pal = this->palette();
  const QColor    c_bg = pal.color(QPalette::Base);
  const QColor    c_fill = pal.color(QPalette::Highlight);
  const QColor    c_text = pal.color(QPalette::Text);
  const QColor    c_border = pal.color(QPalette::Mid);
  const QColor    c_hover = pal.color(QPalette::Highlight);

  // Background + border
  p.setBrush(c_bg);
  p.setPen(QPen(this->is_hovered ? c_hover : c_border,
                this->is_hovered ? this->style.border_width_hovered()
                                 : this->style.border_width()));
  p.drawRoundedRect(this->rect(),
                    this->style.border_radius(),
                    this->style.border_radius());

  // Value fill bar when the range is bounded, drag handle when it is not. Both
  // are drawn in the highlight colour and both define the region over which the
  // label and value are drawn in HighlightedText, so `accent_rect` covers them.
  const bool is_editing = this->value_edit->isVisible();
  const bool has_fill = this->is_range_bounded() && !is_editing;
  const bool has_handle = !this->is_range_bounded() && !is_editing;
  QRect      accent_rect;

  if (has_fill)
  {
    const float r = this->value_to_ratio(this->value);
    const int   rcut = int((1.f - r) * float(this->rect_bar.width()));

    accent_rect = this->rect_bar.adjusted(1, 1, -rcut - 1, -1);

    p.setBrush(c_fill.darker(130));
    p.setPen(Qt::NoPen);

    if (this->add_plus_minus_buttons)
      p.drawRect(accent_rect);
    else
      p.drawRoundedRect(accent_rect,
                        this->style.border_radius(),
                        this->style.border_radius());
  }
  else if (has_handle)
  {
    accent_rect = this->handle_rect();

    // While dragging, mark the rest position the handle will snap back to.
    if (this->is_dragging)
    {
      const int x_mid = this->rect_bar.center().x();
      p.setPen(QPen(c_border, this->style.border_width()));
      p.drawLine(QPoint(x_mid, this->rect_bar.top() + 3),
                 QPoint(x_mid, this->rect_bar.bottom() - 3));
    }

    p.setBrush(c_fill.darker(this->is_dragging ? 110 : 130));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(accent_rect,
                      this->style.border_radius(),
                      this->style.border_radius());
  }

  // +/- button separators
  if (this->add_plus_minus_buttons)
  {
    p.setPen(QPen(c_border, this->style.border_width()));
    p.drawLine(QPoint(this->rect_minus.right() + 1, this->rect().top()),
               QPoint(this->rect_minus.right() + 1, this->rect().bottom()));
    p.drawLine(QPoint(this->rect_plus.left() - 1, this->rect().top()),
               QPoint(this->rect_plus.left() - 1, this->rect().bottom()));
  }

  // Label (left) and value (right) inside the bar
  const QRect   label_rect = this->rect_bar.adjusted(this->base_dx,
                                                   0,
                                                   -this->base_dx,
                                                   0);
  const QString label_str = QString::fromStdString(this->label);
  const QString value_str = QString::fromStdString(this->get_value_as_string());

  if (has_fill || has_handle)
  {
    // Two-pass draw: text over the highlighted fill or handle uses
    // HighlightedText for contrast, text over the plain background uses
    // Text. The two clip regions are complementary (bar minus accent /
    // accent), so nothing is drawn twice.
    const QColor c_highlighted_text = pal.color(QPalette::HighlightedText);

    p.save();
    p.setClipRect(accent_rect);
    p.setBrush(c_highlighted_text);
    p.setPen(c_highlighted_text);
    p.drawText(label_rect, Qt::AlignLeft | Qt::AlignVCenter, label_str);
    p.drawText(label_rect, Qt::AlignRight | Qt::AlignVCenter, value_str);
    p.restore();

    p.save();
    p.setClipRegion(QRegion(this->rect_bar).subtracted(QRegion(accent_rect)));
    p.setBrush(c_text);
    p.setPen(c_text);
    p.drawText(label_rect, Qt::AlignLeft | Qt::AlignVCenter, label_str);
    p.drawText(label_rect, Qt::AlignRight | Qt::AlignVCenter, value_str);
    p.restore();
  }
  else
  {
    p.setBrush(c_text);
    p.setPen(c_text);
    p.drawText(label_rect, Qt::AlignLeft | Qt::AlignVCenter, label_str);
    p.drawText(label_rect, Qt::AlignRight | Qt::AlignVCenter, value_str);
  }

  // ◁/▶ arrows
  p.drawText(this->rect_minus,
             Qt::AlignCenter | Qt::AlignVCenter,
             this->is_minus_hovered ? "◀" : "◁");
  p.drawText(this->rect_plus,
             Qt::AlignCenter | Qt::AlignVCenter,
             this->is_plus_hovered ? "▶" : "▷");
}

void SliderFloat::resizeEvent(QResizeEvent *event)
{
  this->update_geometry();
  QWidget::resizeEvent(event);
}

bool SliderFloat::set_value(float new_value)
{
  new_value = std::clamp(new_value, this->vmin, this->vmax);

  if (new_value == this->value) return false;

  this->value = new_value;
  this->update();
  Q_EMIT this->value_changed();

  if (this->force_edit_ended_emit) Q_EMIT this->edit_ended();

  return true;
}

// ---------------------------------------------------------------------------
// Private slots / helpers
// ---------------------------------------------------------------------------

void SliderFloat::set_is_dragging(bool new_state)
{
  this->is_dragging = new_state;
  this->drag_dx = 0; // an unbounded slider's handle recentres on release
  this->setCursor(new_state ? Qt::SizeHorCursor : Qt::ArrowCursor);
  this->update();
}

QSize SliderFloat::sizeHint() const
{
  return QSize(this->slider_width, this->base_dy);
}

void SliderFloat::update_geometry()
{
  QFontMetrics fm(this->font());
  this->base_dx = fm.horizontalAdvance(QString("M"));
  this->base_dy = fm.height() + this->style.vertical_spacing();

  const int label_w = helpers::text_width(this, this->label);
  this->slider_width = label_w + this->style.horizontal_spacing() +
                       10 * fm.horizontalAdvance(QString("0")) +
                       6 * this->base_dx;

  this->slider_width_min = label_w + this->style.horizontal_spacing() +
                           fm.horizontalAdvance(QString::fromStdString(
                               this->get_value_as_string())) +
                           6 * this->base_dx;

  this->setMinimumWidth(this->slider_width_min);
  this->setMinimumHeight(this->sizeHint().height());
  this->setMaximumHeight(this->sizeHint().height());

  if (this->add_plus_minus_buttons)
  {
    this->rect_minus = this->rect();
    this->rect_minus.setWidth(2 * this->base_dx);

    this->rect_plus = this->rect().adjusted(this->rect().width() -
                                                2 * this->base_dx,
                                            0,
                                            0,
                                            0);
  }
  else
  {
    this->rect_minus = QRect();
    this->rect_plus = QRect();
  }

  const int gap = this->add_plus_minus_buttons ? 2 * this->base_dx : 0;
  this->rect_bar = this->rect().adjusted(gap, 0, -gap, 0);
}

} // namespace meta::qt
