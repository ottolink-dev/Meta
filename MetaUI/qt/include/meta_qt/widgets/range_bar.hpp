/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once

#include <vector>

#include <QWidget>
#include <glm/glm.hpp>

namespace meta::qt
{

struct HistogramData
{
  std::vector<float> x;
  std::vector<float> y;
};

// Horizontal range bar with two draggable handles.
// value.x = low handle, value.y = high handle.
// Invariant: value.x <= value.y, both clamped to [domain_min, domain_max].

class RangeBar : public QWidget
{
  Q_OBJECT

public:
  RangeBar(glm::vec2 &value,
           float      domain_min,
           float      domain_max,
           int        decimals,
           QWidget   *parent = nullptr);

  void set_value(glm::vec2 v);
  void set_histogram(const std::vector<float> &x, const std::vector<float> &y);

Q_SIGNALS:
  void value_changed(glm::vec2 v);
  void drag_ended(glm::vec2 v);

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *) override;

private:
  enum class Handle
  {
    None,
    Low,
    High,
    Track
  };

  QRect  track_rect() const;
  float  canvas_to_value(int px) const;
  int    value_to_canvas(float v) const;
  Handle hit_test(const QPoint &pos) const;
  void   apply_drag(const QPoint &pos);
  void   clamp_and_order();

  glm::vec2 &value_;
  float      domain_min_, domain_max_;
  int        decimals_;

  Handle drag_handle_ = Handle::None;
  Handle hovered_handle_ = Handle::None;
  int    drag_anchor_px_ = 0;    // cursor x at drag start
  float  drag_low_start_ = 0.f;  // value.x at drag start (Track drag)
  float  drag_high_start_ = 0.f; // value.y at drag start (Track drag)

  std::vector<float> hist_x_;
  std::vector<float> hist_y_;

  static constexpr int pad_h_ = 10;   // horizontal padding (handle overhang)
  static constexpr int pad_v_ = 8;    // vertical padding
  static constexpr int handle_w_ = 6; // half-width of a handle rect
  static constexpr int track_h_ = 8;  // track height
};

} // namespace meta::qt