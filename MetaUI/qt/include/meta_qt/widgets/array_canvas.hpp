/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <QWidget>
#include <QImage>
#include <vector>
#include <string>

namespace meta::qt
{

class ArrayCanvas : public QWidget
{
  Q_OBJECT

public:
  explicit ArrayCanvas(const std::string &label,
                       int width = 256,
                       int height = 256,
                       QWidget *parent = nullptr);

  QSize sizeHint() const override;

  void set_field_data(const std::vector<float> &data);
  const std::vector<float> &get_field_data() const;

  void set_background_image(const std::vector<uint8_t> &pixels,
                            int w, int h, int channels);

  void clear();

  int get_field_width() const { return width_; }
  int get_field_height() const { return height_; }

signals:
  void value_changed();
  void edit_ended();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  bool event(QEvent *event) override;

private:
  void draw_at(const QPoint &pos, Qt::MouseButtons buttons);
  void update_geometry();
  bool is_mouse_cursor_on_img() const;
  QColor colormap(float v) const;

  std::string label_;
  int width_ = 128;
  int height_ = 128;

  std::vector<float> field_;

  // Background image
  QImage bg_image_;
  bool show_bg_image_ = true;

  // Brush settings
  int brush_radius_ = 10;
  float brush_strength_ = 0.1f;

  // Modifiers
  bool ctrl_pressed_ = false;
  bool shift_pressed_ = false;
  bool is_drawing_ = false;
  bool is_hovered_ = false;

  QPoint pos_previous_;
  QRect rect_img_;

  std::string help_msg_;
};

} // namespace meta::qt
