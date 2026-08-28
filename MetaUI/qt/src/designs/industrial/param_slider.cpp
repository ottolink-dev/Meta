/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/param_slider.hpp"

#include <algorithm>
#include <cmath>

#include <QLinearGradient>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>

namespace meta::qt::industrial
{

namespace
{
constexpr qreal kLogFloor = 1e-6; ///< below this a log mapping is undefined
}

ParamSlider::ParamSlider(Attribute<float> &attr, const RowContext &ctx, QWidget *parent)
    : Control<float>(ctx, parent)
{
  key_ = attr.name();
  label_ = meta::common::label(attr);
  category_ = meta::common::category(attr);
  min_ = meta::common::min(attr);
  max_ = meta::common::max(attr);
  log_scale_ = meta::common::try_get<bool>(attr, meta::keys::ui::log_scale, false);
  decimals_ = meta::common::try_get_format_decimals(meta::common::format(attr));

  // A log mapping needs a strictly positive lower bound; fall back to linear
  // rather than producing NaNs across the whole rail.
  if (log_scale_ && min_ <= kLogFloor) log_scale_ = false;

  value_ = std::clamp(attr.value(), min_, max_);
  norm_ = to_norm(value_);

  setFixedHeight(theme().metrics.row_height);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  glide_ = new Glide(theme().metrics.glide_ms, this);
  connect(glide_,
          &Glide::tick,
          this,
          [this](qreal t)
          {
            norm_ = t;
            value_ = from_norm(t);
            refresh_field();
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
            value_ = from_norm(t);
            refresh_field();
            update();
            notify_value_changed();
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
            bool        ok = false;
            const float typed = field_->text().toFloat(&ok);
            if (!ok)
            {
              refresh_field(); // reject silently, restore the real value
              return;
            }

            begin_edit();
            glide_->to(to_norm(std::clamp(typed, min_, max_)));
          });

  connect(field_, &QLineEdit::textEdited, this, [this]() { restyle_field(true); });
}

bool ParamSlider::can_render(const Attribute<float> &attr)
{
  // contains_all_keys() is non-const, so probe with find() instead of taking a
  // mutable reference just to ask a question.
  const auto &metadata = attr.metadata();
  if (!metadata.find(meta::keys::constraints::min) ||
      !metadata.find(meta::keys::constraints::max))
    return false;

  return meta::common::max(attr) > meta::common::min(attr);
}

void ParamSlider::set(const float &value)
{
  const float clamped = std::clamp(value, min_, max_);

  // jump() emits tick(), which derives value_ back out of the normalised
  // position -- lossy under a log mapping. Seat the authoritative value after.
  glide_->jump(to_norm(clamped)); // a model sync seats immediately, no glide
  value_ = clamped;

  refresh_field();
  update();
}

QSize ParamSlider::sizeHint() const
{
  return QSize(theme().metrics.label_min_width + 200, theme().metrics.row_height);
}

// --- geometry

int ParamSlider::label_width() const
{
  const Metrics &m = theme().metrics;
  return int(std::clamp<qreal>(width() * m.label_width_ratio,
                               m.label_min_width,
                               m.label_max_width));
}

int ParamSlider::field_width() const
{
  const Metrics &m = theme().metrics;
  // The narrow branch keys off this row's own width, not the window's.
  return width() < m.narrow_threshold ? m.value_field_width_narrow
                                      : m.value_field_width;
}

QRect ParamSlider::rail_rect() const
{
  const Metrics &m = theme().metrics;
  const int      x0 = label_width() + m.gap;
  const int      x1 = width() - field_width() - m.gap;
  const int      y = (height() - m.rail_height) / 2;
  return QRect(x0, y, std::max(0, x1 - x0), m.rail_height);
}

QRect ParamSlider::thumb_rect() const
{
  const Metrics &m = theme().metrics;
  const QRect    rail = rail_rect();
  const int      travel = std::max(0, rail.width() - m.thumb_width);
  const int      x = rail.x() + int(std::round(norm_ * travel));
  const int      y = (height() - m.thumb_height) / 2;
  return QRect(x, y, m.thumb_width, m.thumb_height);
}

// --- value mapping

qreal ParamSlider::to_norm(float value) const
{
  if (max_ <= min_) return 0.0;

  if (log_scale_)
  {
    const qreal lo = std::log(qreal(min_));
    const qreal hi = std::log(qreal(max_));
    const qreal v = std::log(std::max(qreal(value), kLogFloor));
    return std::clamp((v - lo) / (hi - lo), 0.0, 1.0);
  }

  return std::clamp(qreal(value - min_) / qreal(max_ - min_), 0.0, 1.0);
}

float ParamSlider::from_norm(qreal t) const
{
  t = std::clamp(t, 0.0, 1.0);

  if (log_scale_)
  {
    const qreal lo = std::log(qreal(min_));
    const qreal hi = std::log(qreal(max_));
    return float(std::exp(lo + t * (hi - lo)));
  }

  return float(min_ + t * qreal(max_ - min_));
}

// --- painting

void ParamSlider::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const Theme   &t = theme();
  const Metrics &m = t.metrics;
  const bool     locked = is_locked();

  // --- label. Text is the only thing state is allowed to change.
  QFont label_font = ui_font(12, false, 1.0);
  label_font.setCapitalization(QFont::AllUppercase);
  painter.setFont(label_font);
  painter.setPen(t.state_ink(is_modified(), locked));
  const int label_w = label_width();
  painter.drawText(QRect(0, 0, label_w, height()),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   elide_label(QString::fromStdString(label_), label_font, label_w));

  const QRect rail = rail_rect();
  if (rail.width() <= 0) return;

  // --- rail well
  painter.setPen(QPen(t.rail_well_border, 1));
  painter.setBrush(t.rail_well);
  painter.drawRoundedRect(QRectF(rail).adjusted(0.5, 0.5, -0.5, -0.5),
                          m.rail_radius,
                          m.rail_radius);

  // --- fill. Always the group accent; never a state colour.
  const QRect thumb = thumb_rect();
  const int   fill_w = thumb.center().x() - rail.x();
  if (fill_w > 0)
  {
    QRect fill = rail.adjusted(0, 0, 0, 0);
    fill.setWidth(std::min(fill_w, rail.width()));
    painter.setPen(Qt::NoPen);
    painter.setBrush(t.rail_fill(category_, locked));
    painter.drawRoundedRect(QRectF(fill).adjusted(0.5, 0.5, -0.5, -0.5),
                            m.rail_radius,
                            m.rail_radius);
  }

  // --- thumb
  painter.setOpacity(locked ? t.locked_thumb_alpha : 1.0);

  QLinearGradient metal(thumb.topLeft(), thumb.bottomLeft());
  metal.setColorAt(0.0, t.thumb_top);
  metal.setColorAt(1.0, t.thumb_bottom);

  painter.setPen(QPen(t.thumb_border, 1));
  painter.setBrush(metal);
  painter.drawRoundedRect(QRectF(thumb).adjusted(0.5, 0.5, -0.5, -0.5),
                          m.radius,
                          m.radius);

  // grip notch, 2x8 centred
  painter.setPen(Qt::NoPen);
  painter.setBrush(t.thumb_grip);
  painter.drawRect(QRect(thumb.center().x(), thumb.center().y() - 3, 2, 8));

  painter.setOpacity(1.0);
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

  const Metrics &m = theme().metrics;
  const int      fw = field_width();
  field_->setGeometry(width() - fw,
                      (height() - m.value_field_height) / 2,
                      fw,
                      m.value_field_height);

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

  const QRect rail = rail_rect();
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

void ParamSlider::mouseMoveEvent(QMouseEvent *event)
{
  if (!dragging_) return;
  set_from_position(event->pos().x());
}

void ParamSlider::mouseReleaseEvent(QMouseEvent *event)
{
  if (!dragging_) return;

  dragging_ = false;
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
    const float target = std::clamp(std::any_cast<float>(def), min_, max_);
    dragging_ = false;
    begin_edit();
    glide_->to(to_norm(target)); // the reset glides like everything else
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
  const Metrics &m = theme().metrics;
  const QRect    rail = rail_rect();
  const int      travel = std::max(1, rail.width() - m.thumb_width);

  apply_norm(std::clamp(qreal(x - rail.x() - m.thumb_width / 2) / travel, 0.0, 1.0));
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

QString ParamSlider::format_value(float value) const
{
  return QString::number(value, 'f', decimals_);
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
  field_->setStyleSheet(QString("QLineEdit {"
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
