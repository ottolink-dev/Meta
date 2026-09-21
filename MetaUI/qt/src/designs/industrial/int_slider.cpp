/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/int_slider.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>

namespace meta::qt::industrial
{

namespace
{
/// Thumb position an unbounded row rests at: the middle of the rail.
constexpr qreal kRestNorm = 0.5;

/** @brief Drag sensitivity for an unbounded row, in pixels per whole step.
 *
 * Taken from stock SliderInt's PPU_UNBOUNDED rather than picked again, so a
 * Seed feels the same now that it no longer falls through to stock.
 */
constexpr qreal kPixelsPerStep = 4.0;

/// Ctrl divides the step by this, Shift multiplies it. Stock's PPU_MULT_FINE.
constexpr qreal kFineMultiplier = 10.0;
} // namespace

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

  unbounded_ = !has_usable_range(min_, max_);

  // An inverted or empty range cannot clamp, and std::clamp with hi below lo
  // is undefined. Widening to the type's own limits keeps every clamp below
  // well formed; an attribute whose bounds contradict each other is already
  // the unbounded case as far as the rail is concerned.
  if (unbounded_ && !(max_ > min_))
  {
    min_ = std::numeric_limits<int>::lowest();
    max_ = std::numeric_limits<int>::max();
  }

  // The rail may deliberately stop short of what the parameter accepts. Where
  // it does, dragging is held to the rail while typing goes to the real
  // maximum. Declared per attribute rather than inferred: this used to trigger
  // on max == 64 exactly, which caught unrelated parameters whose 64 is a hard
  // cap, Islands and n_vertices among them, and let a user type any number
  // into them.
  input_max_ = max_;
  // An absent override leaves the full range; zero is a valid explicit cap.
  if (const int declared = meta::common::try_get<int>(attr,
                                                      meta::keys::ui::drag_max,
                                                      max_);
      declared > min_ && declared < max_)
  {
    max_ = declared; // the rail ends here; input_max_ keeps the real limit
  }
  value_ = std::clamp(attr.value(), min_, input_max_);
  norm_ = unbounded_ ? kRestNorm : to_norm(value_);

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

            // Unbounded: a thumb settling back to centre is not an edit. The
            // drag published as it went and end_edit() fired on release, so
            // ending one here would close an edit the user has since started.
            if (unbounded_) return;

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

            commit_value(typed);
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

  // The bounds themselves are not screened: a range with no usable span is
  // rendered as a rate handle rather than handed to another design.
  return true;
}

void IntSlider::set(const int &value)
{
  value_ = std::clamp(value, min_, input_max_);

  // Unbounded: the thumb encodes drag distance, not the value, so a sync from
  // the model leaves it where it rests. jump() also cancels a recentre still
  // running, which would otherwise fight the position set here.
  const qreal target = unbounded_ ? kRestNorm : to_norm(value_);

  glide_->jump(target); // a model sync seats immediately
  norm_ = target;
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
  return std::clamp((qreal(value) - min_) / (qreal(max_) - min_), 0.0, 1.0);
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
  visual.unbounded = unbounded_;
  visual.dragging = dragging_;

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

  if (unbounded_)
  {
    // Seat the thumb before the drag is measured. A recentre from the previous
    // drag may still be running, and its finished() would otherwise arrive
    // mid-drag; jump() cancels it without emitting one.
    glide_->jump(kRestNorm);
    norm_ = kRestNorm;
    drag_origin_x_ = event->pos().x();
    value_at_press_ = value_;

    dragging_ = true;
    begin_edit();
    update();

    // Deliberately no set_from_position(): a rate drag measures from where the
    // press landed, so pressing the rail must not move the value at all.
    return;
  }

  dragging_ = true;
  begin_edit();
  set_from_position(event->pos().x());
}

void IntSlider::mouseMoveEvent(QMouseEvent *event)
{
  if (!dragging_) return;

  if (unbounded_)
  {
    drag_by(event->pos().x(), event->modifiers());
    return;
  }

  set_from_position(event->pos().x());
}

void IntSlider::mouseReleaseEvent(QMouseEvent *event)
{
  if (!dragging_) return;

  dragging_ = false;

  if (unbounded_)
  {
    drag_by(event->pos().x(), event->modifiers());

    // The thumb eases back to rest rather than snapping, like everything else
    // in this design. The edit is over as soon as the button is up, though:
    // holding it open for the animation would stall the model sync behind it.
    end_edit();
    glide_->to(kRestNorm);
    return;
  }

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
    commit_value(std::any_cast<int>(def)); // the reset glides where it can
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
  // regardless of how wide its range happens to be. Widened to 64 bits before
  // the clamp because a Seed sits in [0, INT_MAX] and value_ + steps would
  // otherwise overflow at the top of it.
  const long long stepped = static_cast<long long>(value_) + steps;
  commit_value(int(std::clamp<long long>(stepped, min_, max_)));
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

void IntSlider::drag_by(int x, Qt::KeyboardModifiers modifiers)
{
  const int dx = x - drag_origin_x_;

  qreal ppu = kPixelsPerStep;
  if (modifiers & Qt::ControlModifier)
    ppu *= kFineMultiplier;
  else if (modifiers & Qt::ShiftModifier)
    ppu /= kFineMultiplier;

  // The thumb follows the cursor pixel for pixel but stops at the ends of the
  // rail. Its travel is an affordance, not a measurement: the value carries on
  // changing after the thumb has run out of room, which is the whole point of
  // a rate control.
  const SliderGeometry g = SliderGeometry::compute(theme(),
                                                   width(),
                                                   height(),
                                                   norm_);
  const int travel = std::max(1, g.rail.width() - theme().metrics.thumb_width);

  norm_ = std::clamp(kRestNorm + qreal(dx) / qreal(travel), 0.0, 1.0);
  glide_->jump(norm_); // no easing under the cursor, and cancels any recentre

  // Whole steps only, measured from the value at the press rather than
  // accumulated per event: an integer row must never show a fraction, and a
  // drag out and back has to land exactly where it started. Widened to 64 bits
  // because a Seed sits in [0, INT_MAX] and this would overflow near the top.
  const long long moved = static_cast<long long>(qreal(dx) / ppu);
  const long long target = static_cast<long long>(value_at_press_) + moved;

  apply_value(int(std::clamp<long long>(target, min_, max_)), false);
}

void IntSlider::commit_value(int value)
{
  begin_edit();
  apply_value(std::clamp(value, min_, input_max_), !unbounded_);

  // A bounded row ends its edit when the glide settles. An unbounded one has
  // no glide to wait on, so it ends here.
  if (unbounded_) end_edit();
}

void IntSlider::apply_value(int value, bool glide)
{
  const bool changed = value != value_;
  value_ = value;

  // Unbounded: the thumb is a rate affordance rather than a position, so it is
  // never driven from the value. drag_by() owns it and release eases it home.
  if (!unbounded_)
  {
    if (glide)
    {
      glide_->to(to_norm(value_));
    }
    else
    {
      glide_->jump(to_norm(value_)); // a drag tracks the cursor, no glide
      norm_ = to_norm(value_);
    }
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
