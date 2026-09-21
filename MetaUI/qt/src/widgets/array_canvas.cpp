/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/widgets/array_canvas.hpp"
#include "meta_qt/widgets/style.hpp"
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace meta::qt
{

ArrayCanvas::ArrayCanvas(const std::string &label,
                         int                width,
                         int                height,
                         QWidget           *parent)
    : QWidget(parent), label_(label), width_(width), height_(height)
{
  field_.resize(static_cast<size_t>(width_ * height_), 0.f);

  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  setAttribute(Qt::WA_Hover);
  QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  policy.setHeightForWidth(true);
  setSizePolicy(policy);
  setMinimumSize(120, 120);

  help_msg_ =
      "Array editor\n- Left-click: Paint\n- Right-click: Erase\n- Mousewheel: "
      "Brush size\n- Ctrl + Mousewheel: Brush strength\n- Shift + Left-click: "
      "Smooth\n- Key C: Clear canvas\n- Space: Toggle background image";
  setToolTip(QString::fromStdString(help_msg_));

  update_geometry();
}

QSize ArrayCanvas::sizeHint() const { return QSize(320, 320); }

void ArrayCanvas::set_field_data(const std::vector<float> &data)
{
  if (data.size() == field_.size())
  {
    field_ = data;
  }
  else
  {
    std::fill(field_.begin(), field_.end(), 0.f);
    size_t copy_size = std::min(data.size(), field_.size());
    std::copy_n(data.begin(), copy_size, field_.begin());
  }
  update();
}

const std::vector<float> &ArrayCanvas::get_field_data() const { return field_; }

void ArrayCanvas::set_background_image(const std::vector<uint8_t> &pixels,
                                       int                         w,
                                       int                         h,
                                       int                         channels)
{
  if (pixels.empty() || w <= 0 || h <= 0)
  {
    bg_image_ = QImage();
  }
  else
  {
    QImage::Format fmt = (channels == 4)   ? QImage::Format_RGBA8888
                         : (channels == 1) ? QImage::Format_Grayscale8
                                           : QImage::Format_RGB888;
    bg_image_ = QImage(pixels.data(), w, h, w * channels, fmt).copy();
  }
  update_geometry();
  update();
}

void ArrayCanvas::clear()
{
  std::fill(field_.begin(), field_.end(), 0.f);
  update();
  Q_EMIT value_changed();
  Q_EMIT edit_ended();
}

void ArrayCanvas::draw_at(const QPoint &pos, Qt::MouseButtons buttons)
{
  const int   ir = brush_radius_;
  const float sign = (buttons & Qt::LeftButton) ? 1.f : -1.f;

  if (shift_pressed_)
  {
    // Smoothing filter
    for (int j = -ir; j <= ir; ++j)
    {
      for (int i = -ir; i <= ir; ++i)
      {
        int fx = pos.x() + i;
        int fy = pos.y() + j;
        if (fx < 0 || fy < 0 || fx >= width_ || fy >= height_) continue;

        float dist = std::sqrt(static_cast<float>(i * i + j * j));
        if (dist <= static_cast<float>(ir))
        {
          float sum = 0.f;
          int   ns = 0;
          for (int r = -1; r <= 1; ++r)
          {
            for (int s = -1; s <= 1; ++s)
            {
              int nx = fx + r;
              int ny = fy + s;
              if (nx >= 0 && ny >= 0 && nx < width_ && ny < height_)
              {
                sum += field_[static_cast<size_t>(ny * width_ + nx)];
                ns++;
              }
            }
          }
          float  falloff = 1.f - (dist / static_cast<float>(ir));
          float  value_avg = std::clamp(sum / static_cast<float>(ns), 0.f, 1.f);
          size_t idx = static_cast<size_t>(fy * width_ + fx);
          field_[idx] = (1.f - falloff) * field_[idx] + falloff * value_avg;
        }
      }
    }
  }
  else
  {
    // Regular draw/erase
    for (int j = -ir; j <= ir; ++j)
    {
      for (int i = -ir; i <= ir; ++i)
      {
        int fx = pos.x() + i;
        int fy = pos.y() + j;
        if (fx < 0 || fy < 0 || fx >= width_ || fy >= height_) continue;

        float dist = std::sqrt(static_cast<float>(i * i + j * j));
        if (dist <= static_cast<float>(ir))
        {
          float amp = sign * brush_strength_;
          float falloff = 1.f - (dist / static_cast<float>(ir));
          falloff = std::clamp(falloff, 0.f, 1.f);
          size_t idx = static_cast<size_t>(fy * width_ + fx);
          field_[idx] += amp * falloff;
          field_[idx] = std::clamp(field_[idx], 0.f, 1.f);
        }
      }
    }
  }
}

QColor ArrayCanvas::colormap(float v) const
{
  v = std::clamp(v, 0.f, 1.f);
  int val = static_cast<int>(255 * v);
  return QColor(val, val, val);
}

bool ArrayCanvas::is_mouse_cursor_on_img() const
{
  QPoint mouse_pos = mapFromGlobal(QCursor::pos());
  return rect_img_.contains(mouse_pos);
}

void ArrayCanvas::update_geometry()
{
  Style style(this);
  int   gap = style.border_radius();

  const int side = std::max(1, std::min(width(), height()) - 2 * gap);
  rect_img_ = QRect((width() - side) / 2, (height() - side) / 2, side, side);
  update();
}

void ArrayCanvas::resizeEvent(QResizeEvent *event)
{
  if (height() != width()) setFixedHeight(width());
  update_geometry();
  QWidget::resizeEvent(event);
}

QPoint ArrayCanvas::field_position(const QPoint &pos) const
{
  return QPoint(
      int(double(pos.x() - rect_img_.x()) * width_ / rect_img_.width()),
      int(double(pos.y() - rect_img_.y()) * height_ / rect_img_.height()));
}

bool ArrayCanvas::event(QEvent *event)
{
  switch (event->type())
  {
  case QEvent::Enter: setFocus(Qt::MouseFocusReason); break;

  case QEvent::HoverEnter:
    is_hovered_ = true;
    setCursor(Qt::CrossCursor);
    update();
    break;

  case QEvent::HoverLeave:
    is_hovered_ = false;
    setCursor(Qt::ArrowCursor);
    update();
    break;

  case QEvent::HoverMove: update(); break;

  default: break;
  }

  return QWidget::event(event);
}

void ArrayCanvas::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Control)
  {
    ctrl_pressed_ = true;
    update();
  }
  else if (event->key() == Qt::Key_Shift)
  {
    shift_pressed_ = true;
    update();
  }
  else if (event->key() == Qt::Key_C)
  {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Clear canvas",
        "Are you sure you want to clear the canvas?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
      clear();
    }
  }
  else if (event->key() == Qt::Key_Space)
  {
    show_bg_image_ = !show_bg_image_;
    update();
  }
  else
  {
    QWidget::keyPressEvent(event);
  }
}

void ArrayCanvas::keyReleaseEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Control)
  {
    ctrl_pressed_ = false;
    update();
  }
  else if (event->key() == Qt::Key_Shift)
  {
    shift_pressed_ = false;
    update();
  }
  else
  {
    QWidget::keyReleaseEvent(event);
  }
}

void ArrayCanvas::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
  {
    if (!rect_img_.contains(event->position().toPoint())) return;
    is_drawing_ = true;
    QPoint pos = field_position(event->position().toPoint());
    pos_previous_ = pos;
    draw_at(pos, event->buttons());
    update();
    Q_EMIT value_changed();
  }
}

void ArrayCanvas::mouseReleaseEvent(QMouseEvent *event)
{
  if (is_drawing_)
  {
    is_drawing_ = false;
    Q_EMIT edit_ended();
  }
  QWidget::mouseReleaseEvent(event);
}

void ArrayCanvas::mouseMoveEvent(QMouseEvent *event)
{
  if (is_drawing_)
  {
    const QPoint pos = field_position(event->position().toPoint());
    const QPoint delta = pos - pos_previous_;
    const int    steps = std::max(
        1,
        int(std::ceil(std::hypot(delta.x(), delta.y()) /
                      std::max(1., brush_radius_ / 3.))));
    for (int i = 1; i <= steps; ++i)
      draw_at(pos_previous_ + QPoint(qRound(double(delta.x()) * i / steps),
                                     qRound(double(delta.y()) * i / steps)),
              event->buttons());
    pos_previous_ = pos;
    update();
    Q_EMIT value_changed();
  }
  QWidget::mouseMoveEvent(event);
}

void ArrayCanvas::wheelEvent(QWheelEvent *event)
{
  if (is_mouse_cursor_on_img())
  {
    if (event->modifiers() & Qt::ControlModifier)
    {
      float diff = (event->angleDelta().y() > 0 ? 1.f : -1.f) * 0.01f;
      brush_strength_ = std::max(0.01f, brush_strength_ + diff);
    }
    else
    {
      int diff = (event->angleDelta().y() > 0 ? 1 : -1) *
                 std::max(1, brush_radius_ / 8);
      brush_radius_ = std::max(1, brush_radius_ + diff);
    }
    update();
  }
}

void ArrayCanvas::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  Style  style(this);
  int    radius = style.border_radius();
  QColor border_color = is_hovered_ ? palette().color(QPalette::Highlight)
                                    : palette().color(QPalette::Mid);
  int    border_w = is_hovered_ ? style.border_width_hovered()
                                : style.border_width();

  // Background filled area
  QPainterPath outline;
  outline.addRoundedRect(QRectF(rect()), radius, radius);
  painter.setClipPath(outline);
  painter.fillPath(outline, palette().color(QPalette::Base));

  // Background image
  bool is_image = !bg_image_.isNull() && show_bg_image_;
  if (is_image)
  {
    painter.setOpacity(0.5); // alpha overlay
    painter.drawImage(rect_img_, bg_image_);
    painter.setOpacity(1.0);
  }

  // Draw data field
  {
    QImage image(width_, height_, QImage::Format_ARGB32);
    for (int j = 0; j < height_; ++j)
    {
      for (int i = 0; i < width_; ++i)
      {
        float  val = field_[static_cast<size_t>(j * width_ + i)];
        QColor c = colormap(val);
        if (is_image)
        {
          c.setAlphaF(val);
        }
        image.setPixel(i, j, c.rgba());
      }
    }
    painter.drawImage(rect_img_, image);
  }

  // Draw label
  if (!property("industrialEditor").toBool())
  {
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(rect_img_,
                     Qt::AlignLeft | Qt::AlignBottom,
                     QString::fromStdString(label_));
  }

  // Draw brush outline
  if (is_mouse_cursor_on_img())
  {
    QPoint mouse_pos = mapFromGlobal(QCursor::pos());
    QPen   pen(palette().color(QPalette::Highlight), 1);
    if (shift_pressed_)
    {
      pen.setStyle(Qt::DotLine);
    }
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(mouse_pos),
                        double(brush_radius_) * rect_img_.width() / width_,
                        double(brush_radius_) * rect_img_.height() / height_);

    // Info overlay
    QString txt;
    if (ctrl_pressed_)
      txt = QString("Strength: %1")
                .arg(static_cast<double>(brush_strength_), 0, 'f', 2);
    else if (shift_pressed_)
      txt = "Smoothing";

    if (!txt.isEmpty())
      painter.drawText(rect_img_, Qt::AlignRight | Qt::AlignBottom, txt);
  }

  // Draw widget borders
  painter.setPen(QPen(border_color, border_w));
  painter.setBrush(Qt::NoBrush);
  painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), radius, radius);
}

} // namespace meta::qt
