/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <climits>
#include <deque>
#include <string>

#include <QLineEdit>
#include <QWidget>

#include "meta_qt/widgets/style.hpp"

namespace meta::qt
{

class SliderInt : public QWidget
{
  Q_OBJECT

public:
  SliderInt(const std::string &label,
            int                value_init,
            int                vmin,
            int                vmax,
            bool               add_plus_minus_buttons,
            const std::string &value_format, // std::format style, e.g. "{:d}"
            QWidget           *parent = nullptr);

  int         get_value() const;
  std::string get_value_as_string() const;
  bool        set_value(int new_value); // returns true if value changed

  QSize sizeHint() const override;

Q_SIGNALS:
  void value_changed();
  void edit_ended();

protected:
  bool event(QEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  void apply_text_edit_value();
  void set_is_dragging(bool new_state);
  void update_geometry();

  bool  is_range_bounded() const;
  QRect handle_rect() const;

  // --- Config
  Style style{this};

  static constexpr float PPU_UNBOUNDED = 4.f; // px per integer step (unbounded)
  static constexpr float PPU_MULT_FINE = 10.f;

  // --- state ----------------------------------------------------------------
  std::string label;
  int         value_init;
  int         value;
  int         vmin, vmax;
  bool        add_plus_minus_buttons;
  std::string value_format;

  bool is_dragging = false;
  bool is_hovered = false;
  bool is_minus_hovered = false;
  bool is_plus_hovered = false;
  bool is_bar_hovered = false;
  bool force_edit_ended_emit = false;

  int   value_before_dragging = 0;
  int   pos_x_before_dragging = 0;
  float drag_accumulator = 0.f; // sub-integer drag accumulator

  // Cursor displacement from the drag origin, in pixels. Only used to place
  // the handle of an unbounded slider; zeroed whenever a drag starts or ends
  // so the handle always sits at the centre at rest.
  int drag_dx = 0;

  // --- geometry -------------------------------------------------------------
  int base_dx = 8;
  int base_dy = 24;
  int slider_width = 200;
  int slider_width_min = 80;

  QRect rect_minus;
  QRect rect_plus;
  QRect rect_bar;

  // --- child widgets --------------------------------------------------------
  QLineEdit  *value_edit = nullptr;
  std::string style_sheet;
};

} // namespace meta::qt