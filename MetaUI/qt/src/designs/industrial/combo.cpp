/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/combo.hpp"

#include <algorithm>

#include <QApplication>
#include <QDateTime>
#include <QEasingCurve>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QVariantAnimation>

namespace meta::qt::industrial
{

namespace
{

constexpr int kRowHeight = 26;
constexpr int kPopupPadding = 4;
constexpr int kMaxVisibleRows = 12;

/// Timestamp of the last popup close, for the close-then-reopen race.
qint64 g_last_close_ms = 0;

} // namespace

// --- ComboPopup

ComboPopup::ComboPopup(const Theme       &theme,
                       const QStringList &items,
                       int                current,
                       QWidget           *parent)
    : QWidget(parent, Qt::Popup),
      theme_(&theme),
      items_(items),
      current_(current),
      hovered_(current)
{
  // Must be set before the native window is created, which happens on the first
  // show(). Setting it later leaves the unrevealed part of the surface painting
  // opaque black instead of nothing.
  //
  // WA_NoSystemBackground is deliberately *not* set alongside it: together they
  // leave the surface undefined here rather than clear.
  setAttribute(Qt::WA_TranslucentBackground);

  setAttribute(Qt::WA_DeleteOnClose);
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);
}

int ComboPopup::row_height() const { return kRowHeight; }

bool ComboPopup::should_swallow_reopen()
{
  return QDateTime::currentMSecsSinceEpoch() - g_last_close_ms < 200;
}

void ComboPopup::popup_for(const QRect &field_global)
{
  const int visible = std::min<int>(items_.size(), kMaxVisibleRows);
  full_height_ = visible * row_height() + 2 * kPopupPadding;

  const int width = std::max(field_global.width(), 120);

  const QRect screen = QApplication::primaryScreen()->availableGeometry();
  const bool  fits_below = field_global.bottom() + full_height_ <=
                          screen.bottom();

  const int left = field_global.left();
  flipped_ = !fits_below;

  const int y = fits_below ? field_global.bottom() + 2
                           : field_global.top() - 2 - full_height_;

  // The window is created at its final size and never resized. Animating a
  // top-level window is geometry does not work here: the platform enforces a
  // minimum window size and coalesces rapid resizes, so the popup simply snaps
  // to full size, and resizing a native window every frame is expensive anyway.
  // Reveal the card inside a fixed, translucent window instead.
  setGeometry(left, y, width, full_height_);
  show();
  setFocus(Qt::PopupFocusReason);

  open_animation_ = new QVariantAnimation(this);
  open_animation_->setDuration(theme_->metrics.section_ms);
  open_animation_->setEasingCurve(QEasingCurve::OutCubic);
  open_animation_->setStartValue(0);
  open_animation_->setEndValue(full_height_);

  connect(open_animation_,
          &QVariantAnimation::valueChanged,
          this,
          [this](const QVariant &v)
          {
            revealed_ = v.toInt();
            update();
          });

  open_animation_->start();
}

QRect ComboPopup::card_rect() const
{
  const int h = revealed_ > 0 ? revealed_ : full_height_;

  // Opening downward, the card grows from its top edge, which sits against the
  // field. Flipped, it grows upward from its bottom edge, which is the edge
  // touching the field -- otherwise it looks like it falls from the ceiling.
  return flipped_ ? QRect(0, full_height_ - h, width(), h)
                  : QRect(0, 0, width(), h);
}

int ComboPopup::index_at(const QPoint &pos) const
{
  if (!rect().contains(pos)) return -1;

  const int index = (pos.y() - kPopupPadding) / row_height();
  return index >= 0 && index < items_.size() ? index : -1;
}

void ComboPopup::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const Theme &t = *theme_;
  const QRect  card = card_rect();

  // Everything is clipped to the revealed card, so the rows stay put and are
  // uncovered rather than sliding. Laying them out against the animating height
  // would read as the list scrolling instead of opening.
  painter.setClipRect(card);

  // One surface, painted once: frame and rows come from the same pass so they
  // cannot disagree about colour part-way through opening.
  painter.setPen(QPen(t.hairline, 1));
  painter.setBrush(t.bar);
  painter.drawRoundedRect(QRectF(card).adjusted(0.5, 0.5, -0.5, -0.5),
                          t.metrics.radius,
                          t.metrics.radius);

  painter.setFont(ui_font(12));

  for (int i = 0; i < items_.size(); ++i)
  {
    const QRect row(1,
                    kPopupPadding + i * row_height(),
                    width() - 2,
                    row_height());
    if (!row.intersects(card)) continue;

    if (i == hovered_)
    {
      painter.setPen(Qt::NoPen);
      painter.setBrush(t.section_header);
      painter.drawRect(row);
    }

    painter.setPen(i == current_ ? t.accent : t.ink_primary);
    painter.drawText(row.adjusted(10, 0, -10, 0),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     items_.at(i));
  }
}

void ComboPopup::mouseMoveEvent(QMouseEvent *event)
{
  const int index = index_at(event->pos());
  if (index != hovered_)
  {
    hovered_ = index;
    update();
  }
}

void ComboPopup::mousePressEvent(QMouseEvent *event)
{
  // Overriding this at all suppresses Qt's built-in "press outside dismisses",
  // so an outside press has to be handled here.
  if (!rect().contains(event->pos()))
  {
    close();
    return;
  }

  QWidget::mousePressEvent(event);
}

void ComboPopup::mouseReleaseEvent(QMouseEvent *event)
{
  const int index = index_at(event->pos());
  if (index >= 0)
  {
    Q_EMIT selected(index);
    close();
    return;
  }

  if (!rect().contains(event->pos())) close();
}

void ComboPopup::keyPressEvent(QKeyEvent *event)
{
  switch (event->key())
  {
  case Qt::Key_Down:
    hovered_ = std::min<int>(hovered_ + 1, items_.size() - 1);
    update();
    return;
  case Qt::Key_Up:
    hovered_ = std::max(hovered_ - 1, 0);
    update();
    return;
  case Qt::Key_Return:
  case Qt::Key_Enter:
    if (hovered_ >= 0) Q_EMIT selected(hovered_);
    close();
    return;
  case Qt::Key_Escape: close(); return;
  default: break;
  }

  QWidget::keyPressEvent(event);
}

void ComboPopup::hideEvent(QHideEvent *event)
{
  // hideEvent rather than destroyed(): WA_DeleteOnClose defers deletion by an
  // event-loop pass, by which point the dismissing click has already been
  // processed and reopened the popup.
  g_last_close_ms = QDateTime::currentMSecsSinceEpoch();
  QWidget::hideEvent(event);
}

// --- shared field painting

void paint_combo_field(QWidget       &widget,
                       const Theme   &theme,
                       const QString &label,
                       const QString &value,
                       bool           open,
                       bool           modified,
                       bool           locked)
{
  QPainter painter(&widget);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const Metrics &m = theme.metrics;
  const int      height = widget.height();

  const int label_width = int(
      std::clamp<qreal>(widget.width() * m.label_width_ratio,
                        m.label_min_width,
                        m.label_max_width));

  QFont label_font = ui_font(12, false, 1.0);
  label_font.setCapitalization(QFont::AllUppercase);
  painter.setFont(label_font);
  painter.setPen(theme.state_ink(modified, locked));
  painter.drawText(QRect(0, 0, label_width, height),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   label);

  const QRect field(label_width + m.gap,
                    (height - m.value_field_height) / 2,
                    widget.width() - label_width - m.gap,
                    m.value_field_height);

  painter.setOpacity(locked ? theme.locked_thumb_alpha : 1.0);

  // An open combo takes the accent border, matching an editing value field.
  painter.setPen(QPen(open ? theme.accent : theme.field_border, 1));
  painter.setBrush(theme.field);
  painter.drawRoundedRect(QRectF(field).adjusted(0.5, 0.5, -0.5, -0.5),
                          m.radius,
                          m.radius);

  painter.setFont(ui_font(12));
  painter.setPen(theme.ink_primary);
  painter.drawText(field.adjusted(8, 0, -22, 0),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   painter.fontMetrics().elidedText(value,
                                                    Qt::ElideRight,
                                                    field.width() - 30));

  // chevron
  const QPoint centre(field.right() - 12, field.center().y());
  QPainterPath chevron;
  chevron.moveTo(centre.x() - 4, centre.y() - 2);
  chevron.lineTo(centre.x(), centre.y() + 2);
  chevron.lineTo(centre.x() + 4, centre.y() - 2);

  painter.setBrush(Qt::NoBrush);
  painter.setPen(QPen(theme.ink_icon, 1.5));
  painter.drawPath(chevron);

  painter.setOpacity(1.0);
}

// --- EnumCombo

EnumCombo::EnumCombo(Attribute<int>   &attr,
                     const RowContext &ctx,
                     QWidget          *parent)
    : Control<int>(ctx, parent)
{
  label_ = meta::common::label(attr);
  items_ = meta::common::enum_items<int>(attr);
  value_ = attr.value();

  setFixedHeight(theme().metrics.row_height);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

bool EnumCombo::can_render(const Attribute<int> &attr)
{
  return !meta::common::enum_items<int>(attr).empty();
}

void EnumCombo::set(const int &value)
{
  value_ = value;
  update();
}

QSize EnumCombo::sizeHint() const
{
  return QSize(theme().metrics.label_min_width + 160,
               theme().metrics.row_height);
}

void EnumCombo::paintEvent(QPaintEvent *)
{
  QString text;
  for (const auto &[v, name] : items_)
    if (v == value_) text = QString::fromStdString(name);

  paint_combo_field(*this,
                    theme(),
                    QString::fromStdString(label_),
                    text,
                    open_,
                    is_modified(),
                    is_locked());
}

void EnumCombo::mouseReleaseEvent(QMouseEvent *event)
{
  // Open on release. Opening on press means the matching release lands outside
  // the new popup and dismisses it immediately.
  if (is_locked() || event->button() != Qt::LeftButton) return;
  if (ComboPopup::should_swallow_reopen()) return;

  open_popup();
}

void EnumCombo::open_popup()
{
  QStringList names;
  int         current = -1;
  for (int i = 0; i < int(items_.size()); ++i)
  {
    names << QString::fromStdString(items_.at(i).second);
    if (items_.at(i).first == value_) current = i;
  }

  auto *popup = new ComboPopup(theme(), names, current, this);

  connect(popup,
          &ComboPopup::selected,
          this,
          [this](int index)
          {
            if (index < 0 || index >= int(items_.size())) return;

            value_ = items_.at(index).first;
            update();

            begin_edit();
            notify_value_changed();
            end_edit();
          });

  connect(popup,
          &QObject::destroyed,
          this,
          [this]()
          {
            open_ = false;
            update();
          });

  open_ = true;
  update();

  const Metrics &m = theme().metrics;
  const int   label_width = int(std::clamp<qreal>(width() * m.label_width_ratio,
                                                m.label_min_width,
                                                m.label_max_width));
  const QRect field(label_width + m.gap,
                    (height() - m.value_field_height) / 2,
                    width() - label_width - m.gap,
                    m.value_field_height);

  popup->popup_for(QRect(mapToGlobal(field.topLeft()), field.size()));
}

// --- StringCombo

StringCombo::StringCombo(Attribute<std::string> &attr,
                         const RowContext       &ctx,
                         QWidget                *parent)
    : Control<std::string>(ctx, parent)
{
  label_ = meta::common::label(attr);
  items_ = meta::common::allowed_values<std::string>(attr);
  value_ = attr.value();

  setFixedHeight(theme().metrics.row_height);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

bool StringCombo::can_render(const Attribute<std::string> &attr)
{
  return !meta::common::allowed_values<std::string>(attr).empty();
}

void StringCombo::set(const std::string &value)
{
  value_ = value;
  update();
}

QSize StringCombo::sizeHint() const
{
  return QSize(theme().metrics.label_min_width + 160,
               theme().metrics.row_height);
}

void StringCombo::paintEvent(QPaintEvent *)
{
  paint_combo_field(*this,
                    theme(),
                    QString::fromStdString(label_),
                    QString::fromStdString(value_),
                    open_,
                    is_modified(),
                    is_locked());
}

void StringCombo::mouseReleaseEvent(QMouseEvent *event)
{
  if (is_locked() || event->button() != Qt::LeftButton) return;
  if (ComboPopup::should_swallow_reopen()) return;

  open_popup();
}

void StringCombo::open_popup()
{
  QStringList names;
  int         current = -1;
  for (int i = 0; i < int(items_.size()); ++i)
  {
    names << QString::fromStdString(items_.at(i));
    if (items_.at(i) == value_) current = i;
  }

  auto *popup = new ComboPopup(theme(), names, current, this);

  connect(popup,
          &ComboPopup::selected,
          this,
          [this](int index)
          {
            if (index < 0 || index >= int(items_.size())) return;

            value_ = items_.at(index);
            update();

            begin_edit();
            notify_value_changed();
            end_edit();
          });

  connect(popup,
          &QObject::destroyed,
          this,
          [this]()
          {
            open_ = false;
            update();
          });

  open_ = true;
  update();

  const Metrics &m = theme().metrics;
  const int   label_width = int(std::clamp<qreal>(width() * m.label_width_ratio,
                                                m.label_min_width,
                                                m.label_max_width));
  const QRect field(label_width + m.gap,
                    (height() - m.value_field_height) / 2,
                    width() - label_width - m.gap,
                    m.value_field_height);

  popup->popup_for(QRect(mapToGlobal(field.topLeft()), field.size()));
}

} // namespace meta::qt::industrial
