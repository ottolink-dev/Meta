/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/int_slider.hpp"

#include <algorithm>
#include <cmath>

#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>

namespace meta::qt::industrial
{

IntSlider::IntSlider(Attribute<int>   &attr,
                     const RowContext &ctx,
                     QWidget          *parent)
    : Control<int>(ctx, parent)
{
  key_ = attr.name();
  label_ = meta::common::label(attr);
  category_ = meta::common::category(attr);
  min_ = meta::common::min(attr);
  max_ = meta::common::max(attr);

  value_ = std::clamp(attr.value(), min_, max_);
  norm_ = to_norm(value_);

  setFixedHeight(theme().metrics.row_height);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  glide_ = new Glide(theme().metrics.glide_ms, this);

  // The glide drives the painted position only. value_ is set up front by
  // apply_value(), so neither the readout nor the model ever shows an
  // intermediate fractional value that an int attribute cannot hold.
  connect(glide_,
          &Glide::tick,
          this,
          [this](qreal t)
          {
            norm_ = t;
            update();
          });

  connect(glide_,
          &Glide::finished,
          this,
          [this](qreal t)
          {
            norm_ = t;
            update();
            end_edit();
          });
  glide_->jump(norm_);

  field_ = new QLineEdit(this);
  field_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  field_->setFrame(false);
  field_->setFont(mono_font(13));
  field_->installEventFilter(this);
  refresh_field();
  restyle_field();

  connect(field_,
          &QLineEdit::editingFinished,
          this,
          [this]()
          {
            bool      ok = false;
            const int typed = field_->text().toInt(&ok);
            if (!ok)
            {
              refresh_field(); // reject silently, restore the real value
              return;
            }

            begin_edit();
            apply_value(std::clamp(typed, min_, max_), true);
          });

  connect(field_,
          &QLineEdit::textEdited,
          this,
          [this]() { restyle_field(true); });
}

bool IntSlider::can_render(const Attribute<int> &attr)
{
  const auto &metadata = attr.metadata();
  if (!metadata.find(meta::keys::constraints::min) ||
      !metadata.find(meta::keys::constraints::max))
    return false;

  // Declines unbounded ranges so they fall through to the stock input, which is
  // the right control for a number with no limits.
  return has_usable_range(meta::common::min(attr), meta::common::max(attr));
}

void IntSlider::set(const int &value)
{
  value_ = std::clamp(value, min_, max_);
  glide_->jump(to_norm(value_)); // a model sync seats immediately
  norm_ = to_norm(value_);
  refresh_field();
  update();
}

QSize IntSlider::sizeHint() const
{
  return QSize(theme().metrics.label_min_width + 200,
               theme().metrics.row_height);
}

qreal IntSlider::to_norm(int value) const
{
  if (max_ <= min_) return 0.0;
  return std::clamp(qreal(value - min_) / qreal(max_ - min_), 0.0, 1.0);
}

int IntSlider::from_norm(qreal t) const
{
  // Round rather than truncate, or the top step is unreachable and dragging
  // right never quite arrives at max.
  return int(std::lround(min_ + std::clamp(t, 0.0, 1.0) * qreal(max_ - min_)));
}

void IntSlider::paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  const SliderGeometry geometry = SliderGeometry::compute(theme(),
                                                          width(),
                                                          height(),
                                                          norm_);

  SliderVisual visual;
  visual.category = category_;
  visual.modified = is_modified();
  visual.locked = is_locked();

  QFont label_font = row_label_font();
  visual.label = elide_label(QString::fromStdString(label_),
                             label_font,
                             geometry.label.width());

  paint_slider_row(painter, theme(), geometry, visual, height());
}

void IntSlider::resizeEvent(QResizeEvent *event)
{
  if (event->oldSize().width() == event->size().width())
  {
    QWidget::resizeEvent(event);
    return;
  }

  field_->setGeometry(
      SliderGeometry::compute(theme(), width(), height(), norm_).field);

  QWidget::resizeEvent(event);
}

void IntSlider::mousePressEvent(QMouseEvent *event)
{
  if (is_locked() || event->button() != Qt::LeftButton)
  {
    event->ignore();
    return;
  }

  const QRect rail = SliderGeometry::compute(theme(), width(), height(), norm_)
                         .rail;
  if (!rail.adjusted(-4, -10, 4, 10).contains(event->pos()))
  {
    event->ignore();
    return;
  }

  setFocus(Qt::MouseFocusReason);
  dragging_ = true;
  begin_edit();
  set_from_position(event->pos().x());
}

void IntSlider::mouseMoveEvent(QMouseEvent *event)
{
  if (!dragging_) return;
  set_from_position(event->pos().x());
}

void IntSlider::mouseReleaseEvent(QMouseEvent *event)
{
  if (!dragging_) return;

  dragging_ = false;
  set_from_position(event->pos().x());
  end_edit();
}

void IntSlider::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (is_locked()) return;

  const auto &provider = context().default_value;
  if (!provider) return;

  const std::any def = provider(key_);
  if (!def.has_value()) return;

  try
  {
    dragging_ = false;
    begin_edit();
    apply_value(std::clamp(std::any_cast<int>(def), min_, max_), true);
  }
  catch (const std::bad_any_cast &)
  {
    // A default of the wrong type is a host bug, not a reason to misbehave.
  }

  event->accept();
}

void IntSlider::handle_wheel(QWheelEvent *event)
{
  const int steps = event->angleDelta().y() / 120;
  if (steps == 0)
  {
    event->ignore();
    return;
  }

  // One notch is one unit, which is what an integer control should do
  // regardless of how wide its range happens to be.
  begin_edit();
  apply_value(std::clamp(value_ + steps, min_, max_), true);
  event->accept();
}

void IntSlider::on_state_changed()
{
  restyle_field(field_ && field_->hasFocus());
  update();
}

bool IntSlider::eventFilter(QObject *watched, QEvent *event)
{
  if (watched == field_ &&
      (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut))
  {
    const bool editing = event->type() == QEvent::FocusIn;
    if (!editing) refresh_field();
    restyle_field(editing);
  }

  return Control<int>::eventFilter(watched, event);
}

void IntSlider::set_from_position(int x)
{
  const Metrics       &m = theme().metrics;
  const SliderGeometry g = SliderGeometry::compute(theme(),
                                                   width(),
                                                   height(),
                                                   norm_);
  const int            travel = std::max(1, g.rail.width() - m.thumb_width);

  const qreal t = std::clamp(qreal(x - g.rail.x() - m.thumb_width / 2) / travel,
                             0.0,
                             1.0);

  // Quantise to the integer the rail actually represents, so the thumb sits on
  // whole values during a drag rather than between them.
  apply_value(from_norm(t), false);
}

void IntSlider::apply_value(int value, bool glide)
{
  const bool changed = value != value_;
  value_ = value;

  if (glide)
  {
    glide_->to(to_norm(value_));
  }
  else
  {
    glide_->jump(to_norm(value_)); // a drag tracks the cursor, no glide
    norm_ = to_norm(value_);
  }

  refresh_field();
  update();

  if (changed) notify_value_changed();
}

void IntSlider::refresh_field()
{
  if (!field_ || field_->hasFocus()) return; // never overwrite mid-typing

  const QSignalBlocker blocker(field_);
  field_->setText(QString::number(value_));
}

void IntSlider::restyle_field(bool editing)
{
  if (!field_) return;

  field_->setReadOnly(is_locked());
  field_->setStyleSheet(
      field_stylesheet(theme(), editing, is_modified(), is_locked()));
}

} // namespace meta::qt::industrial
