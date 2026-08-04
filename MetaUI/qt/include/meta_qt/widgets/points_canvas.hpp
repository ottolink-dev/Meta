/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once

#include <cstdint>
#include <vector>

#include <QWidget>
#include <glm/glm.hpp>

namespace meta::qt
{

// ---------------------------------------------------------------------------
// PointsCanvas
//
// Displays and edits a std::vector<glm::vec3> where:
//   .x, .y  — 2-D position in [min_x,max_x] × [min_y,max_y]
//   .z      — scalar value forced to [0, 1], colour-mapped on the canvas
//
// Mouse interactions:
//   Left-click empty area     → add point (z = 1.0)
//   Left-drag existing point  → move it
//   Right-click point         → delete it
//   Scroll wheel on point     → adjust z in [0, 1] by z_step
struct ImageData
{
  int                  width;
  int                  height;
  int                  channels;
  std::vector<uint8_t> pixels;
};

class PointsCanvas : public QWidget
{
  Q_OBJECT

public:
  enum class Mode
  {
    Points, // unordered cloud — click anywhere to add
    Path // ordered sequence — click appends to end, points connected in order
  };

  PointsCanvas(std::vector<glm::vec3> &points,
               float                   min_x,
               float                   max_x,
               float                   min_y,
               float                   max_y,
               float                   z_step,
               Mode                    mode = Mode::Points,
               bool                    closed = false,
               QWidget                *parent = nullptr);

  void clear_all();
  void randomize(int count);
  void load_csv(const QString &path); // x,y,z per line (z clamped to [0,1])
  void set_points(const std::vector<glm::vec3> &new_points);
  void set_background_image(const std::vector<uint8_t> &pixels, int w, int h, int channels);

Q_SIGNALS:
  void points_changed();
  void drag_ended();

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *) override;
  void wheelEvent(QWheelEvent *e) override;

private:
  QRect     canvas_rect() const;
  QPoint    value_to_canvas(float x, float y) const;
  glm::vec2 canvas_to_value(const QPoint &p) const;
  int       hit_test(const QPoint &pos) const;

  // Returns index i such that a new point should be inserted at i+1,
  // or -1 if no segment is close enough. Path mode only.
  int    segment_hit_test(const QPoint &pos) const;
  QColor z_to_color(float z) const; // blue(0) → red(1)

  std::vector<glm::vec3> &points_;
  float                   min_x_, max_x_, min_y_, max_y_, z_step_;

  int  hovered_idx_ = -1;
  int  drag_idx_ = -1;
  bool moved_during_drag_ = false;
  int  hovered_segment_ = -1;

  Mode mode_;
  bool closed_;

  std::vector<uint8_t> bg_pixels_;
  int                  bg_w_ = 0, bg_h_ = 0, bg_channels_ = 0;

  static constexpr int   PAD = 10;
  static constexpr float HIT_R = 8.f;
  static constexpr float POINT_R = 5.f;
};

} // namespace meta::qt