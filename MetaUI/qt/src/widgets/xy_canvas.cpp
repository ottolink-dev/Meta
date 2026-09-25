/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

#include <algorithm>
#include <cmath>

#include <QFontDatabase>
#include <QMouseEvent>
#include <QPainter>

#include "meta_qt/widgets/xy_canvas.hpp"

namespace meta::qt
{

XYCanvas::XYCanvas(glm::vec2 &value,
                   float      min_x,
                   float      max_x,
                   float      min_y,
                   float      max_y,
                   bool       show_grid,
                   QWidget   *parent)
    : QWidget(parent),
      value_(value),
      min_x_(min_x),
      max_x_(max_x),
      min_y_(min_y),
      max_y_(max_y),
      show_grid_(show_grid)
{
  setMinimumSize(160, 100);
  QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  policy.setHeightForWidth(true);
  setSizePolicy(policy);
  setFixedHeight(heightForWidth(240));
  setMouseTracking(true);
  setCursor(Qt::CrossCursor);
}

void XYCanvas::apply_pos(const QPoint &pos)
{
  const glm::vec2 nv = canvas_to_value(pos);
  if (nv.x != value_.x || nv.y != value_.y)
  {
    value_.x = nv.x;
    value_.y = nv.y;
    update();
    Q_EMIT value_changed(value_);
  }
}

glm::vec2 XYCanvas::canvas_to_value(const QPoint &pos) const
{
  const QRect r = padded_rect();
  const float tx = float(pos.x() - r.left()) / float(r.width());
  const float ty = float(r.bottom() - pos.y()) / float(r.height()); // Y flipped
  const float vx = tx * (max_x_ - min_x_) + min_x_;
  const float vy = ty * (max_y_ - min_y_) + min_y_;
  return {std::clamp(vx, min_x_, max_x_), std::clamp(vy, min_y_, max_y_)};
}

void XYCanvas::mouseMoveEvent(QMouseEvent *e)
{
  if (dragging_) apply_pos(e->pos());
}

void XYCanvas::mousePressEvent(QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton)
  {
    dragging_ = true;
    apply_pos(e->pos());
  }
}

void XYCanvas::mouseReleaseEvent(QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton && dragging_)
  {
    dragging_ = false;
    apply_pos(e->pos());
    Q_EMIT drag_ended(value_);
    update();
  }
}

// as tall as the plane needs at this width (its aspect), 120..max_side_
int XYCanvas::heightForWidth(int w) const
{
  const qreal plane_w = std::max(10, w - 2 * pad_);
  const int   h = int(std::lround(plane_w / plane_aspect_)) + 2 * pad_;
  return std::clamp(h, 120, max_side_);
}

QSize XYCanvas::sizeHint() const { return QSize(240, heightForWidth(240)); }

void XYCanvas::set_plane_aspect(float aspect)
{
  plane_aspect_ = (std::isfinite(aspect) && aspect > 0.f) ? std::clamp(aspect, 0.1f, 10.f)
                                                          : 1.f;
  setFixedHeight(heightForWidth(width() > 0 ? width() : 240));
  update();
}

void XYCanvas::resizeEvent(QResizeEvent *e)
{
  QWidget::resizeEvent(e);

  // height follows width; a fixed height keeps layouts that do not ask for
  // height-for-width (section cards measuring their content) in step too
  const int h = heightForWidth(width());
  if (std::abs(height() - h) > 1) setFixedHeight(h);
}

// the plane: the largest rectangle of the plane's aspect that fits inside the
// padding, centred
QRect XYCanvas::padded_rect() const
{
  const qreal room_w = std::max(10, width() - 2 * pad_);
  const qreal room_h = std::max(10, height() - 2 * pad_);
  qreal       w = room_w;
  qreal       h = w / plane_aspect_;
  if (h > room_h)
  {
    h = room_h;
    w = h * plane_aspect_;
  }
  return QRect(int((width() - w) / 2.0), int((height() - h) / 2.0), int(w), int(h));
}


void XYCanvas::paintEvent(QPaintEvent *)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  const QRect  r = padded_rect();
  const QRectF rf = QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5);
  const QColor base = palette().color(QPalette::Base);
  const QColor mid = palette().color(QPalette::Mid);
  const QColor text = palette().color(QPalette::Text);
  const QColor accent = palette().color(QPalette::Highlight);

  const auto faded = [](QColor c, qreal a)
  {
    c.setAlphaF(a);
    return c;
  };

  // plane: rounded well
  p.setPen(QPen(faded(mid, 0.9), 1));
  p.setBrush(base);
  p.drawRoundedRect(rf, 6, 6);
  p.setClipRect(r.adjusted(1, 1, -1, -1));

  // grid
  if (show_grid_)
  {
    p.setPen(QPen(faded(mid, 0.45), 1, Qt::DotLine));
    constexpr int div = 4;
    for (int i = 1; i < div; ++i)
    {
      const qreal gx = r.left() + r.width() * i / qreal(div);
      const qreal gy = r.top() + r.height() * i / qreal(div);
      p.drawLine(QPointF(gx, r.top()), QPointF(gx, r.bottom()));
      p.drawLine(QPointF(r.left(), gy), QPointF(r.right(), gy));
    }
  }

  // axes through origin (only when origin falls inside the domain)
  p.setPen(QPen(faded(mid, 0.9), 1));
  if (min_x_ <= 0 && 0 <= max_x_)
  {
    const int ax = value_to_canvas_x(0, r);
    p.drawLine(ax, r.top(), ax, r.bottom());
  }
  if (min_y_ <= 0 && 0 <= max_y_)
  {
    const int ay = value_to_canvas_y(0, r);
    p.drawLine(r.left(), ay, r.right(), ay);
  }

  // crosshair through the current value
  const int cx = value_to_canvas_x(value_.x, r);
  const int cy = value_to_canvas_y(value_.y, r);
  p.setPen(QPen(faded(accent, dragging_ ? 0.75 : 0.45), 1));
  p.drawLine(cx, r.top(), cx, r.bottom());
  p.drawLine(r.left(), cy, r.right(), cy);

  // the point: halo while dragging, accent disc with a light ring
  if (dragging_)
  {
    p.setPen(Qt::NoPen);
    p.setBrush(faded(accent, 0.25));
    p.drawEllipse(QPointF(cx, cy), point_r_ + 6.0, point_r_ + 6.0);
  }
  p.setPen(QPen(faded(text, 0.95), 1.5));
  p.setBrush(accent);
  p.drawEllipse(QPointF(cx, cy), point_r_, point_r_);

  // coordinates: a small pill in the bottom-left corner
  {
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPixelSize(11);
    p.setFont(font);
    const QString lbl = QString("%1, %2")
                            .arg(double(value_.x), 0, 'f', 3)
                            .arg(double(value_.y), 0, 'f', 3);
    const QFontMetrics fm(font);
    const QRectF pill(r.left() + 6,
                      r.bottom() - 6 - (fm.height() + 4),
                      fm.horizontalAdvance(lbl) + 12,
                      fm.height() + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(faded(base.darker(130), 0.85));
    p.drawRoundedRect(pill, pill.height() / 2, pill.height() / 2);
    p.setPen(faded(text, 0.85));
    p.drawText(pill, Qt::AlignCenter, lbl);
  }
}

void XYCanvas::set_value(glm::vec2 v)
{
  value_.x = std::clamp(v.x, min_x_, max_x_);
  value_.y = std::clamp(v.y, min_y_, max_y_);
  update();
}

int XYCanvas::value_to_canvas_x(float vx, const QRect &r) const
{
  const float t = float(vx - min_x_) / float(max_x_ - min_x_);
  return r.left() + int(t * r.width());
}

int XYCanvas::value_to_canvas_y(float vy, const QRect &r) const
{
  const float t = float(vy - min_y_) / float(max_y_ - min_y_);
  return r.bottom() - int(t * r.height()); // Y flipped
}

} // namespace meta::qt
