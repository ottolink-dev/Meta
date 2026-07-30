/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

#include <algorithm>
#include <format>
#include <random>

#include <QFontMetrics>
#include <QHoverEvent>
#include <QMenu>
#include <QPainter>

#include "meta/logger.hpp"

#include "meta_qt/widgets/helpers.hpp"
#include "meta_qt/widgets/slider_int.hpp"

namespace meta::qt
{

SliderInt::SliderInt(const std::string &label_,
                     int                value_init_,
                     int                vmin_,
                     int                vmax_,
                     bool               add_plus_minus_buttons_,
                     const std::string &value_format_,
                     QWidget           *parent)
    : QWidget(parent),
      value_init(value_init_),
      value(value_init_),
      vmin(vmin_),
      vmax(vmax_),
      add_plus_minus_buttons(add_plus_minus_buttons_),
      value_format(value_format_)
{
  this->label = helpers::truncate_string(label_, this->style.label_max_len());

  this->setMouseTracking(true);
  this->setAttribute(Qt::WA_Hover);
  this->setContextMenuPolicy(Qt::CustomContextMenu);

  this->update_geometry();
  this->connect(this,
                &SliderInt::edit_ended,
                [this]() { this->update_geometry(); });

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
                &SliderInt::apply_text_edit_value);
}

void SliderInt::apply_text_edit_value()
{
  bool ok = false;
  int  new_value = this->value_edit->text().toInt(&ok);
  if (ok && this->set_value(new_value)) Q_EMIT this->edit_ended();

  this->value_edit->setVisible(false);
  this->update();
}

bool SliderInt::event(QEvent *event)
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

int SliderInt::get_value() const { return this->value; }

std::string SliderInt::get_value_as_string() const
{
  return std::vformat(this->value_format, std::make_format_args(this->value));
}

void SliderInt::mouseDoubleClickEvent(QMouseEvent *)
{
  const bool is_bounded = this->is_range_bounded();
  const int  delta = is_bounded ? std::max(1,
                                          (this->vmax - this->vmin) /
                                              this->style.button_ticks())
                                : 1;

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
    if (this->set_value(this->value - delta)) Q_EMIT this->edit_ended();
  }
  else if (this->is_plus_hovered)
  {
    if (this->set_value(this->value + delta)) Q_EMIT this->edit_ended();
  }
}

void SliderInt::mouseMoveEvent(QMouseEvent *event)
{
  if (!this->is_dragging)
  {
    QWidget::mouseMoveEvent(event);
    return;
  }

  // For integer sliders we accumulate sub-integer motion to avoid jitter
  // when the range is large relative to the bar width.
  float ppu;

  if (!this->is_range_bounded())
    ppu = PPU_UNBOUNDED;
  else
    ppu = float(this->rect_bar.width()) / float(this->vmax - this->vmin);

  const Qt::KeyboardModifiers mods = event->modifiers();
  this->force_edit_ended_emit = false;

  if ((mods & Qt::ControlModifier) && (mods & Qt::ShiftModifier))
    this->force_edit_ended_emit = true;
  else if (mods & Qt::ControlModifier)
    ppu *= PPU_MULT_FINE;
  else if (mods & Qt::ShiftModifier)
    ppu /= PPU_MULT_FINE;

  const int dx = event->position().toPoint().x() - this->pos_x_before_dragging;
  const float dv = float(dx) / ppu;

  // Accumulate fractional motion; only commit whole integer steps.
  this->drag_dx = dx;
  this->drag_accumulator = dv;
  const int delta = static_cast<int>(this->drag_accumulator);

  if (delta != 0) this->set_value(this->value_before_dragging + delta);

  this->update(); // the handle moves even between integer steps

  QWidget::mouseMoveEvent(event);
}

void SliderInt::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    const bool is_bounded = this->is_range_bounded();
    const int  delta = is_bounded ? std::max(1,
                                            (this->vmax - this->vmin) /
                                                this->style.button_ticks())
                                  : 1;

    if (this->is_bar_hovered)
    {
      this->value_before_dragging = this->value;
      this->pos_x_before_dragging = event->position().toPoint().x();
      this->drag_accumulator = 0.f;
      this->set_is_dragging(true);
    }
    else if (this->is_minus_hovered)
    {
      if (this->set_value(this->value - delta)) Q_EMIT this->edit_ended();
    }
    else if (this->is_plus_hovered)
    {
      if (this->set_value(this->value + delta)) Q_EMIT this->edit_ended();
    }
  }
}

void SliderInt::mouseReleaseEvent(QMouseEvent *event)
{
  if (this->is_dragging && event->button() == Qt::LeftButton)
  {
    this->set_is_dragging(false);
    if (this->value != this->value_before_dragging) Q_EMIT this->edit_ended();
  }
}

bool SliderInt::is_range_bounded() const
{
  return this->vmin != INT_MIN && this->vmax != INT_MAX &&
         this->vmax > this->vmin;
}

// Handle of an unbounded slider: centred at rest, following the drag while one
// is in progress, clamped so it never leaves the track. Its travel is only an
// affordance - the value keeps changing once the handle hits an end.
QRect SliderInt::handle_rect() const
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

void SliderInt::paintEvent(QPaintEvent *)
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

  // Value fill bar when the range is bounded, drag handle when it is not: an
  // unbounded range has no ratio to fill, so the bar would otherwise stay
  // empty whatever the value.
  const bool is_editing = this->value_edit->isVisible();

  if (this->is_range_bounded() && !is_editing)
  {
    const int   range = this->vmax - this->vmin;
    const float r = float(this->value - this->vmin) / float(range);
    const int   rcut = int((1.f - r) * float(this->rect_bar.width()));

    p.setBrush(c_fill.darker(130));
    p.setPen(Qt::NoPen);

    if (this->add_plus_minus_buttons)
      p.drawRect(this->rect_bar.adjusted(1, 1, -rcut - 1, -1));
    else
      p.drawRoundedRect(this->rect_bar.adjusted(1, 1, -rcut - 1, -1),
                        this->style.border_radius(),
                        this->style.border_radius());
  }
  else if (!is_editing)
  {
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
    p.drawRoundedRect(this->handle_rect(),
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

  // Label + value
  p.setBrush(c_text);
  p.setPen(c_text);

  const QRect label_rect = this->rect_bar.adjusted(this->base_dx,
                                                   0,
                                                   -this->base_dx,
                                                   0);
  p.drawText(label_rect,
             Qt::AlignLeft | Qt::AlignVCenter,
             QString::fromStdString(this->label));
  p.drawText(label_rect,
             Qt::AlignRight | Qt::AlignVCenter,
             QString::fromStdString(this->get_value_as_string()));

  // ◁/▶ arrows
  p.drawText(this->rect_minus,
             Qt::AlignCenter | Qt::AlignVCenter,
             this->is_minus_hovered ? "◀" : "◁");
  p.drawText(this->rect_plus,
             Qt::AlignCenter | Qt::AlignVCenter,
             this->is_plus_hovered ? "▶" : "▷");
}

void SliderInt::resizeEvent(QResizeEvent *event)
{
  this->update_geometry();
  QWidget::resizeEvent(event);
}

void SliderInt::set_is_dragging(bool new_state)
{
  this->is_dragging = new_state;
  if (!new_state) this->drag_accumulator = 0.f; // reset on release
  this->drag_dx = 0; // an unbounded slider's handle recentres on release
  this->setCursor(new_state ? Qt::SizeHorCursor : Qt::ArrowCursor);
  this->update();
}

bool SliderInt::set_value(int new_value)
{
  new_value = std::clamp(new_value, this->vmin, this->vmax);

  if (new_value == this->value) return false;

  this->value = new_value;
  this->update();
  Q_EMIT this->value_changed();

  if (this->force_edit_ended_emit) Q_EMIT this->edit_ended();

  return true;
}

QSize SliderInt::sizeHint() const
{
  return QSize(this->slider_width, this->base_dy);
}

void SliderInt::update_geometry()
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
