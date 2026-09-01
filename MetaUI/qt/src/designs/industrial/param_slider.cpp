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

  const SliderGeometry geometry = SliderGeometry::compute(theme(),
                                                          width(),
                                                          height(),
                                                          norm_);

  SliderVisual visual;
  visual.category = category_;
  visual.modified = is_modified();
  visual.locked = is_locked();

  QFont label_font = ui_font(12, false, 1.0);
  label_font.setCapitalization(QFont::AllUppercase);
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

  field_->setGeometry(SliderGeometry::compute(theme(), width(), height(), norm_).field);

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

  const QRect rail = SliderGeometry::compute(theme(), width(), height(), norm_).rail;
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
  const Metrics       &m = theme().metrics;
  const SliderGeometry g = SliderGeometry::compute(theme(), width(), height(), norm_);
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
