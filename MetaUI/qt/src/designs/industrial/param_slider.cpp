/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/param_slider.hpp"
#include <format>

#include "meta_qt/ui/number_format.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QLineEdit>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>

namespace meta::qt::industrial
{

namespace
{
constexpr qreal kLogFloor = 1e-6; ///< below this a log mapping is undefined

/// Thumb position an unbounded row rests at: the middle of the rail.
constexpr qreal kRestNorm = 0.5;

/** @brief Drag sensitivity for an unbounded row, in pixels per unit.
 *
 * Taken from stock SliderFloat's PPU_F rather than picked again, so a row that
 * used to fall through to stock feels the same now that it does not.
 */
constexpr qreal kPixelsPerUnit = 200.0;

/// Ctrl divides the step by this, Shift multiplies it. Stock's PPU_MULT_FINE.
constexpr qreal kFineMultiplier = 10.0;
} // namespace

ParamSlider::ParamSlider(Attribute<float> &attr,
                         const RowContext &ctx,
                         QWidget          *parent)
    : Control<float>(ctx, parent)
{
  key_ = attr.name();
  label_ = meta::common::label(attr);
  category_ = meta::common::category(attr);
  min_ = meta::common::min(attr);
  max_ = meta::common::max(attr);
  log_scale_ = meta::common::try_get<bool>(attr,
                                           meta::keys::ui::log_scale,
                                           false);
  format_ = meta::common::format(attr);
  decimals_ = meta::common::try_get_format_decimals(format_);

  unbounded_ = !has_usable_range(min_, max_);

  // An inverted or empty range cannot clamp, and std::clamp with hi below lo
  // is undefined. Widening to the type's own limits keeps every clamp below
  // well formed; an attribute whose bounds contradict each other is already
  // the unbounded case as far as the rail is concerned.
  if (unbounded_ && !(max_ > min_))
  {
    min_ = std::numeric_limits<float>::lowest();
    max_ = std::numeric_limits<float>::max();
  }

  // Only an unbounded range defeats a log mapping outright, because there is
  // no span to lay one over. A minimum of zero does not: the domain is simply
  // clamped up to kLogFloor, which is what stock's SliderFloat has always
  // done. Bailing to linear here instead meant every log attribute starting at
  // zero silently drew as linear, which is nearly all of them.
  if (log_scale_ && unbounded_) log_scale_ = false;

  // The rail may deliberately stop short of what the parameter accepts. Where
  // it does, dragging is held to the rail while typing goes to the real
  // maximum. Declared per attribute rather than inferred: this used to trigger
  // on max == 64 exactly, which caught unrelated parameters whose 64 is a hard
  // cap, Islands and n_vertices among them, and let a user type any number
  // into them.
  input_max_ = max_;
  // An absent override leaves the full range; zero is a valid explicit cap.
  if (const float declared = meta::common::try_get<float>(
          attr,
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
  connect(glide_,
          &Glide::tick,
          this,
          [this](qreal t)
          {
            norm_ = t;

            // Unbounded: the glide carries the thumb home after a drag and
            // does nothing else. Deriving the value from the thumb here would
            // drag it back towards the centre along with the thumb.
            if (!unbounded_)
            {
              value_ = from_norm(t);
              refresh_field();
            }

            update();
          });

  // The commit rides the animation's completion. Emitting it when the glide
  // starts would publish the value the control still held a frame ago.
  connect(glide_,
          &Glide::finished,
          this,
          [this](qreal t)
          {
            norm_ = t;
            update();

            // Unbounded: a thumb settling back to centre is not an edit. The
            // drag published as it went and end_edit() fired on release, so a
            // commit here would raise a second edit out of an animation the
            // user has already let go of.
            if (unbounded_) return;

            value_ = from_norm(t);
            refresh_field();
            notify_value_changed();
            end_edit();
          });
  {
    const QSignalBlocker blocker(glide_);
    glide_->jump(norm_);
  }

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
            bool        ok = false;
            const float typed = field_->text().toFloat(&ok);
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

bool ParamSlider::can_render(const Attribute<float> &attr)
{
  // contains_all_keys() is non-const, so probe with find() instead of taking a
  // mutable reference just to ask a question.
  const auto &metadata = attr.metadata();
  if (!metadata.find(meta::keys::constraints::min) ||
      !metadata.find(meta::keys::constraints::max))
    return false;

  // The bounds themselves are not screened: a range with no usable span is
  // rendered as a rate handle rather than handed to another design.
  return true;
}

void ParamSlider::set(const float &value)
{
  const float clamped = std::clamp(value, min_, input_max_);

  // Unbounded: the thumb encodes drag distance, not the value, so a sync from
  // the model leaves it where it rests. jump() also cancels a recentre still
  // running, which would otherwise fight the position set here.
  glide_->jump(unbounded_ ? kRestNorm : to_norm(clamped));

  // jump() emits tick(), which derives value_ back out of the normalised
  // position -- lossy under a log mapping. Seat the authoritative value after.
  value_ = clamped;

  refresh_field();
  update();
}

QSize ParamSlider::sizeHint() const
{
  return QSize(theme().metrics.label_min_width + 200,
               theme().metrics.row_height);
}

// --- value mapping

qreal ParamSlider::to_norm(float value) const
{
  if (max_ <= min_) return 0.0;

  if (log_scale_)
  {
    // Every endpoint is floored, not just the value. log(0) is negative
    // infinity, and a minimum of zero is the common case rather than the
    // exotic one, so leaving lo unfloored produced a NaN across the whole rail
    // and was why this fell back to linear instead.
    const qreal lo = std::log(std::max(qreal(min_), kLogFloor));
    const qreal hi = std::log(std::max(qreal(max_), kLogFloor));
    const qreal v = std::log(std::max(qreal(value), kLogFloor));

    if (hi <= lo) return 0.0;

    return std::clamp((v - lo) / (hi - lo), 0.0, 1.0);
  }

  return std::clamp(qreal(value - min_) / qreal(max_ - min_), 0.0, 1.0);
}

float ParamSlider::from_norm(qreal t) const
{
  t = std::clamp(t, 0.0, 1.0);

  if (log_scale_)
  {
    const qreal lo = std::log(std::max(qreal(min_), kLogFloor));
    const qreal hi = std::log(std::max(qreal(max_), kLogFloor));

    // Snap the bottom of the rail back to the real minimum. The floor is a
    // device for making the mapping well defined, and without this a rail
    // declared from zero would bottom out at 1e-6 instead of at zero.
    if (t <= 0.0) return min_;

    return float(std::exp(lo + t * (hi - lo)));
  }

  return float(min_ + t * qreal(max_ - min_));
}

// --- painting

void ParamSlider::paintEvent(QPaintEvent *)
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

void ParamSlider::resizeEvent(QResizeEvent *event)
{
  // Nothing here derives height from width, so a pure-height resize would be a
  // wasted layout pass. Bail before touching child geometry.
  if (event->oldSize().width() == event->size().width())
  {
    QWidget::resizeEvent(event);
    return;
  }

  field_->setGeometry(
      SliderGeometry::compute(theme(), width(), height(), norm_).field);

  QWidget::resizeEvent(event);
}

// --- interaction

void ParamSlider::mousePressEvent(QMouseEvent *event)
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

void ParamSlider::mouseMoveEvent(QMouseEvent *event)
{
  if (!dragging_) return;

  if (unbounded_)
  {
    drag_by(event->pos().x(), event->modifiers());
    return;
  }

  set_from_position(event->pos().x());
}

void ParamSlider::mouseReleaseEvent(QMouseEvent *event)
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

void ParamSlider::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (is_locked()) return;

  const auto &provider = context().default_value;
  if (!provider) return;

  const std::any def = provider(key_);
  if (!def.has_value()) return;

  try
  {
    dragging_ = false;
    commit_value(std::any_cast<float>(def)); // the reset glides where it can
  }
  catch (const std::bad_any_cast &)
  {
    // A default of the wrong type is a host bug, not a reason to misbehave.
  }

  event->accept();
}

void ParamSlider::handle_wheel(QWheelEvent *event)
{
  const int steps = event->angleDelta().y() / 120;
  if (steps == 0)
  {
    event->ignore();
    return;
  }

  if (unbounded_)
  {
    // One unit per notch. A percentage of the rail is the wrong measure when
    // the rail represents no span, and it is what stock does here too.
    commit_value(value_ + float(steps));
    event->accept();
    return;
  }

  // One notch moves 1% of the rail, which stays sane under a log mapping.
  begin_edit();
  glide_->to(std::clamp(norm_ + steps * 0.01, 0.0, 1.0));
  event->accept();
}

void ParamSlider::on_state_changed()
{
  restyle_field(field_ && field_->hasFocus());
  update();
}

// --- helpers

bool ParamSlider::eventFilter(QObject *watched, QEvent *event)
{
  if (watched == field_ &&
      (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut))
  {
    // hasFocus() is not yet settled while the event is being delivered, so the
    // event type is the authority on which way the transition goes.
    const bool editing = event->type() == QEvent::FocusIn;
    if (!editing) refresh_field();
    restyle_field(editing);
  }

  return Control<float>::eventFilter(watched, event);
}

void ParamSlider::set_from_position(int x)
{
  const Metrics       &m = theme().metrics;
  const SliderGeometry g = SliderGeometry::compute(theme(),
                                                   width(),
                                                   height(),
                                                   norm_);
  const int            travel = std::max(1, g.rail.width() - m.thumb_width);

  apply_norm(
      std::clamp(qreal(x - g.rail.x() - m.thumb_width / 2) / travel, 0.0, 1.0));
}

void ParamSlider::apply_norm(qreal t)
{
  // A drag tracks the cursor directly; gliding here would lag the pointer.
  glide_->jump(t);
  norm_ = t;
  value_ = from_norm(t);
  refresh_field();
  update();
  notify_value_changed();
}

void ParamSlider::drag_by(int x, Qt::KeyboardModifiers modifiers)
{
  const int dx = x - drag_origin_x_;

  qreal ppu = kPixelsPerUnit;
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

  // Measured from the value at the press rather than accumulated per event, so
  // a drag out and back returns to exactly where it started.
  apply_value(float(qreal(value_at_press_) + qreal(dx) / ppu));
}

void ParamSlider::apply_value(float value)
{
  const float clamped = std::clamp(value, min_, input_max_);
  const bool  changed = clamped != value_;

  value_ = clamped;
  refresh_field(); // runs even unchanged, to normalise what was typed
  update();

  if (changed) notify_value_changed();
}

void ParamSlider::commit_value(float value)
{
  begin_edit();

  const float clamped = std::clamp(value, min_, input_max_);

  if (!unbounded_)
  {
    // Typed numbers are authoritative, even where normalising a wide range
    // cannot represent all their digits. Position the rail, then seat the
    // value.
    glide_->jump(to_norm(clamped));
  }

  value_ = clamped;
  refresh_field();
  update();
  notify_value_changed();
  end_edit();
}

QString ParamSlider::format_value(float value) const
{
  // Honour the presentation type the attribute declared. A rate spanning five
  // decades declares "{:.2e}" precisely so its readout stays three characters
  // of mantissa and an exponent; rendering it fixed gives 0.00000100, which
  // does not fit the value field and reads as a truncated number rather than
  // a small one.
  //
  // Only e and g are routed here. A fixed spec goes to display_float, which
  // widens the precision rather than letting a small non-zero value round away
  // to 0.00, and that is the behaviour a fixed readout wants.
  if (has_exponent_format(format_))
  {
    try
    {
      return QString::fromStdString(
          std::vformat(format_, std::make_format_args(value)));
    }
    catch (const std::format_error &)
    {
      // A malformed spec is a host bug, not a reason to render nothing.
    }
  }

  return display_float(value, decimals_);
}

void ParamSlider::refresh_field()
{
  if (!field_ || field_->hasFocus()) return; // never overwrite mid-typing

  const QSignalBlocker blocker(field_);
  field_->setText(format_value(value_));
}

void ParamSlider::restyle_field(bool editing)
{
  if (!field_) return;

  const Theme &t = theme();

  const QColor bg = editing ? t.field_editing : t.field;
  const QColor border = editing ? t.accent : t.field_border;

  field_->setReadOnly(is_locked());
  field_->setStyleSheet(
      QString("QLineEdit {"
              " background: %1;"
              " border: 1px solid %2;"
              " border-radius: %3px;"
              " color: %4;"
              " padding-right: 4px;"
              "}")
          .arg(bg.name())
          .arg(border.name())
          .arg(t.metrics.radius)
          .arg(t.state_ink(is_modified(), is_locked()).name()));
}

} // namespace meta::qt::industrial
