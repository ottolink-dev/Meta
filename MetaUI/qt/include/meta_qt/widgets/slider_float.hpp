/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once

#include <cfloat>
#include <deque>
#include <string>

#include <QLineEdit>
#include <QWidget>

#include "meta_qt/widgets/style.hpp"

namespace meta::qt
{

// ---------------------------------------------------------------------------
// SliderFloat
//
// A single-row float slider with:
//   • Drag on the bar to change the value (Ctrl = fine, Shift = coarse,
//     Ctrl+Shift = fine + immediate edit_ended per step)
//   • A proportional fill bar when the range is bounded; when it is not
//     (vmin/vmax at ±FLT_MAX) there is no ratio to fill, so the bar instead
//     shows a handle that follows the drag and springs back to the centre on
//     release — a directional drag counter rather than a slider
//   • Double-click the bar to type a value directly
//   • Optional ◁/▷ increment buttons on the sides
//   • Right-click context menu: Reset, Randomize (when bounded), History
//
// Signals:
//   value_changed()  — emitted on every incremental change
//   edit_ended()     — emitted on drag release, text confirm, button click
// ---------------------------------------------------------------------------

class SliderFloat : public QWidget
{
  Q_OBJECT

public:
  SliderFloat(
      const std::string &label,
      float              value_init,
      float              vmin,
      float              vmax,
      bool               add_plus_minus_buttons,
      const std::string &value_format, // std::format style, e.g. "{:.3f}",
      bool               log_scale = false,
      QWidget           *parent = nullptr);

  float       get_value() const;
  std::string get_value_as_string() const;
  bool        set_value(float new_value); // returns true if value changed

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

  float value_to_ratio(float v) const;
  float ratio_to_value(float r) const;

  bool  is_range_bounded() const;
  QRect handle_rect() const;

  // --- Config (replaces QSX_CONFIG) TODO / use Qt QStyle
  Style style{this};

  static constexpr float PPU_F = 200.f; // pixels-per-unit (unbounded)
  static constexpr float PPU_MULT_FINE = 10.f;

  // --- State
  std::string label;
  float       value_init;
  float       value;
  float       vmin, vmax;
  bool        add_plus_minus_buttons;
  std::string value_format;
  bool        log_scale = false;

  bool is_dragging = false;
  bool is_hovered = false;
  bool is_minus_hovered = false;
  bool is_plus_hovered = false;
  bool is_bar_hovered = false;
  bool force_edit_ended_emit = false;

  float value_before_dragging = 0.f;
  int   pos_x_before_dragging = 0;

  // Cursor displacement from the drag origin, in pixels. Only used to place
  // the handle of an unbounded slider; zeroed whenever a drag starts or ends
  // so the handle always sits at the centre at rest.
  int drag_dx = 0;

  std::deque<float> history;

  // --- geometry
  int base_dx = 8;
  int base_dy = 24;
  int slider_width = 200;
  int slider_width_min = 80;

  QRect rect_minus;
  QRect rect_plus;
  QRect rect_bar;

  // --- child widgets
  QLineEdit  *value_edit = nullptr;
  std::string style_sheet;
};

} // namespace meta::qt