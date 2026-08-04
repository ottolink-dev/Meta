/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <algorithm>

#include <QFontDatabase>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>

#include "meta_qt/widgets/range_bar.hpp"

namespace meta::qt
{

RangeBar::RangeBar(glm::vec2 &value,
                   float      domain_min,
                   float      domain_max,
                   int        decimals,
                   QWidget   *parent)
    : QWidget(parent),
      value_(value),
      domain_min_(domain_min),
      domain_max_(domain_max),
      decimals_(decimals)
{
  setFixedHeight(44);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setMouseTracking(true);
}

void RangeBar::apply_drag(const QPoint &pos)
{
  const int   dx = pos.x() - drag_anchor_px_;
  const float dv = float(dx) / float(track_rect().width()) *
                   (domain_max_ - domain_min_);
  const float span = drag_high_start_ - drag_low_start_;

  switch (drag_handle_)
  {
  case Handle::Low:
    value_.x = std::clamp(canvas_to_value(pos.x()), domain_min_, value_.y);
    break;

  case Handle::High:
    value_.y = std::clamp(canvas_to_value(pos.x()), value_.x, domain_max_);
    break;

  case Handle::Track:
  {
    // Shift the whole range, clamping at both edges while preserving span.
    float lo = drag_low_start_ + dv;
    float hi = drag_high_start_ + dv;
    if (lo < domain_min_)
    {
      lo = domain_min_;
      hi = lo + span;
    }
    if (hi > domain_max_)
    {
      hi = domain_max_;
      lo = hi - span;
    }
    value_.x = lo;
    value_.y = hi;
    break;
  }

  default: return;
  }

  update();
  Q_EMIT value_changed(value_);
}

float RangeBar::canvas_to_value(int px) const
{
  const QRect tr = track_rect();
  const float t = float(px - tr.left()) / float(tr.width());
  return domain_min_ + std::clamp(t, 0.f, 1.f) * (domain_max_ - domain_min_);
}

void RangeBar::clamp_and_order()
{
  value_.x = std::clamp(value_.x, domain_min_, domain_max_);
  value_.y = std::clamp(value_.y, domain_min_, domain_max_);
  if (value_.x > value_.y) std::swap(value_.x, value_.y);
}

RangeBar::Handle RangeBar::hit_test(const QPoint &pos) const
{
  const int lx = value_to_canvas(value_.x);
  const int hx = value_to_canvas(value_.y);
  const int ht = track_h_ + 6; // match drawn handle height
  const int cy = track_rect().center().y();

  // Check handles first (priority over track).
  if (std::abs(pos.x() - lx) <= handle_w_ + 2 &&
      std::abs(pos.y() - cy) <= ht / 2)
    return Handle::Low;

  if (std::abs(pos.x() - hx) <= handle_w_ + 2 &&
      std::abs(pos.y() - cy) <= ht / 2)
    return Handle::High;

  // Check filled track segment (drag whole range).
  if (pos.x() >= lx && pos.x() <= hx &&
      std::abs(pos.y() - cy) <= track_h_ / 2 + 2)
    return Handle::Track;

  return Handle::None;
}

void RangeBar::leaveEvent(QEvent *)
{
  hovered_handle_ = Handle::None;
  update();
}

void RangeBar::mouseMoveEvent(QMouseEvent *e)
{
  if (drag_handle_ != Handle::None)
  {
    apply_drag(e->pos());
  }
  else
  {
    const Handle prev = hovered_handle_;
    hovered_handle_ = hit_test(e->pos());
    setCursor(hovered_handle_ != Handle::None ? Qt::SizeHorCursor
                                              : Qt::ArrowCursor);
    if (hovered_handle_ != prev) update();
  }
}

void RangeBar::mousePressEvent(QMouseEvent *e)
{
  if (e->button() != Qt::LeftButton) return;

  drag_handle_ = hit_test(e->pos());
  drag_anchor_px_ = e->pos().x();
  drag_low_start_ = value_.x;
  drag_high_start_ = value_.y;

  if (drag_handle_ != Handle::None) setCursor(Qt::SizeHorCursor);
}

void RangeBar::mouseReleaseEvent(QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton && drag_handle_ != Handle::None)
  {
    apply_drag(e->pos());
    drag_handle_ = Handle::None;
    setCursor(Qt::ArrowCursor);
    Q_EMIT drag_ended(value_);
    update();
  }
}

void RangeBar::paintEvent(QPaintEvent *)
{
  // Draw histogram if present
  if (!this->hist_y_.empty())
  {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    const QRect r = this->rect();
    const float ymax = *std::max_element(this->hist_y_.begin(),
                                         this->hist_y_.end());
    if (ymax > 0.f)
    {
      const int   n = static_cast<int>(this->hist_y_.size());
      const float bw = static_cast<float>(r.width()) / static_cast<float>(n);
      QColor      c = this->palette().color(QPalette::Mid);
      c.setAlpha(90);
      painter.setPen(Qt::NoPen);
      painter.setBrush(c);
      for (int i = 0; i < n; ++i)
      {
        const float h = (this->hist_y_[i] / ymax) * r.height();
        painter.drawRect(QRectF(r.left() + i * bw, r.bottom() - h, bw, h));
      }
    }
  }

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  const QRect tr = track_rect();
  const int   lx = value_to_canvas(value_.x);
  const int   hx = value_to_canvas(value_.y);

  // Track background
  p.setPen(Qt::NoPen);
  p.setBrush(palette().color(QPalette::Mid));
  p.drawRoundedRect(tr, 3, 3);

  // Filled section between handles
  if (hx > lx)
  {
    QRect filled(lx, tr.top(), hx - lx, tr.height());
    p.setBrush(palette().color(QPalette::Highlight).darker(110));
    p.drawRect(filled);
  }

  // Track border
  p.setPen(QPen(palette().color(QPalette::Dark), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(tr, 3, 3);

  // Draw a handle: a rounded rectangle straddling the track vertically
  auto draw_handle = [&](int x, bool hovered, bool dragged)
  {
    const QRect hr(x - handle_w_, tr.top() - 3, handle_w_ * 2, tr.height() + 6);
    p.setPen(QPen(palette().color(QPalette::Dark), 1));
    p.setBrush(dragged   ? palette().color(QPalette::Highlight)
               : hovered ? palette().color(QPalette::Light)
                         : palette().color(QPalette::Button));
    p.drawRoundedRect(hr, 3, 3);
  };

  draw_handle(lx, hovered_handle_ == Handle::Low, drag_handle_ == Handle::Low);
  draw_handle(hx,
              hovered_handle_ == Handle::High,
              drag_handle_ == Handle::High);

  // Labels: low value left of low handle, high value right of high handle,
  // span in the center of the filled section.
  p.setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  p.setPen(palette().color(QPalette::Text));

  const QString lo_txt = QString::number(double(value_.x), 'f', decimals_);
  const QString hi_txt = QString::number(double(value_.y), 'f', decimals_);

  // Low label — left-aligned below the low handle
  p.drawText(QRect(tr.left(), tr.bottom() + 3, (lx - tr.left()) * 2, 16),
             Qt::AlignLeft | Qt::AlignTop,
             lo_txt);

  // High label — right-aligned below the high handle
  p.drawText(
      QRect(hx - (tr.right() - hx), tr.bottom() + 3, (tr.right() - hx) * 2, 16),
      Qt::AlignRight | Qt::AlignTop,
      hi_txt);
}

void RangeBar::set_value(glm::vec2 v)
{
  value_ = v;
  clamp_and_order();
  update();
}

void RangeBar::set_histogram(const std::vector<float> &x,
                             const std::vector<float> &y)
{
  this->hist_x_ = x;
  this->hist_y_ = y;
  this->update();
}

QRect RangeBar::track_rect() const
{
  const int cy = height() / 2 -
                 4; // slight upward bias to leave room for labels
  return QRect(pad_h_, cy - track_h_ / 2, width() - 2 * pad_h_, track_h_);
}

int RangeBar::value_to_canvas(float v) const
{
  const QRect tr = track_rect();
  const float t = (v - domain_min_) / (domain_max_ - domain_min_);
  return tr.left() + int(std::clamp(t, 0.f, 1.f) * float(tr.width()));
}

} // namespace meta::qt
