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

  // Histogram — drawn after the track so the track does not cover it. Bins go
  // through value_to_canvas() like the handles, so the bin under a handle
  // corresponds to that handle's value.
  const float ymax = hist_y_.empty()
                         ? 0.f
                         : *std::max_element(hist_y_.begin(), hist_y_.end());
  if (ymax > 0.f)
  {
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(Qt::NoPen);

    const int  n = static_cast<int>(hist_y_.size());
    const bool use_x = static_cast<int>(hist_x_.size()) == n;

    // Bin center in value space; uniform spacing over the domain when no
    // matching hist_x_ is available.
    auto center = [&](int i)
    {
      return use_x ? hist_x_[static_cast<size_t>(i)]
                   : domain_min_ + (float(i) + 0.5f) / float(n) *
                                       (domain_max_ - domain_min_);
    };

    QColor c_out = palette().color(QPalette::Text);
    c_out.setAlpha(110);
    QColor c_in = palette().color(QPalette::Highlight);
    c_in.setAlpha(180);

    for (int i = 0; i < n; ++i)
    {
      // Bin edges sit halfway to the neighbouring centers.
      const float cm = center(i);
      float       e0, e1;
      if (n == 1)
      {
        e0 = domain_min_;
        e1 = domain_max_;
      }
      else if (i == 0)
      {
        e1 = 0.5f * (cm + center(1));
        e0 = cm - (e1 - cm);
      }
      else if (i == n - 1)
      {
        e0 = 0.5f * (center(i - 1) + cm);
        e1 = cm + (cm - e0);
      }
      else
      {
        e0 = 0.5f * (center(i - 1) + cm);
        e1 = 0.5f * (cm + center(i + 1));
      }

      const int x0 = value_to_canvas(e0);
      const int x1 = std::max(value_to_canvas(e1), x0 + 1);

      const float h = (hist_y_[static_cast<size_t>(i)] / ymax) *
                      float(rect().height());
      const bool in_sel = cm >= value_.x && cm <= value_.y;
      p.setBrush(in_sel ? c_in : c_out);
      p.drawRect(QRectF(x0, rect().bottom() - h, x1 - x0, h));
    }

    p.setRenderHint(QPainter::Antialiasing, true);
  }
  else if (hist_provided_)
  {
    // A provider ran but produced nothing (empty or flat input) — say so
    // instead of being indistinguishable from a broken widget.
    QColor c = palette().color(QPalette::Text);
    c.setAlpha(120);
    QFont f = p.font();
    f.setItalic(true);
    p.setFont(f);
    p.setPen(c);
    p.drawText(QRect(tr.left(), 0, tr.width(), tr.top() - 2),
               Qt::AlignHCenter | Qt::AlignBottom,
               QObject::tr("no histogram data"));
  }

  // Track border
  p.setPen(QPen(palette().color(QPalette::Dark), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(tr, 3, 3);

  // Draw a handle: a rounded rectangle straddling the track vertically,
  // outlined with the text color so it stays visible on the track.
  auto draw_handle = [&](int x, bool hovered, bool dragged)
  {
    const QRect hr(x - handle_w_, tr.top() - 3, handle_w_ * 2, tr.height() + 6);
    p.setPen(QPen(palette().color(QPalette::Text), 1));
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
  this->hist_provided_ = true;
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
