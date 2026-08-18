/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <array>
#include <string>
#include <vector>

#include <QWidget>

#include "meta/ext/color_gradient/color_gradient.hpp"

class QScrollArea;

namespace meta::qt
{

class PresetGridWidget;

// ---------------------------------------------------------------------------
// GradientBarWidget
//
// Custom-painted gradient bar with interactive stop handles.
// ---------------------------------------------------------------------------

class GradientBarWidget : public QWidget
{
  Q_OBJECT

public:
  static constexpr int BAR_H = 28;
  static constexpr int STOP_R = 6;
  static constexpr int PAD = 10;
  static constexpr int TOTAL_H = PAD + BAR_H + STOP_R * 2 + 6;
  static constexpr int RADIUS = 4;

  explicit GradientBarWidget(std::vector<Stop> &stops,
                             QWidget           *parent = nullptr);

  void sort_stops();

Q_SIGNALS:
  void value_changed();
  void edit_ended();

protected:
  void paintEvent(QPaintEvent *) override;
  void mouseDoubleClickEvent(QMouseEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void contextMenuEvent(QContextMenuEvent *e) override;

private:
  QRectF bar_rect() const;
  QRectF stop_rect(const Stop &s) const;
  int    hit_test(const QPoint &pos) const;

  std::vector<Stop> &stops_;
  int                selected_idx_ = -1;
  bool               dragging_ = false;
};

// ---------------------------------------------------------------------------
// GradientPicker
//
// A self-contained gradient editor widget.
//
// Top:    Gradient bar with draggable stops (double-click to add/edit,
//         drag to move, right-click to remove).
// Bottom: Vertically scrollable grid of preset swatches.
//
// The entire content is held within a vertical QScrollArea so that reducing
// the widget height never squashes or clips the gradient visualization.
// ---------------------------------------------------------------------------

class GradientPicker : public QWidget
{
  Q_OBJECT

public:
  explicit GradientPicker(std::vector<Stop>         &stops,
                          const std::vector<Preset> &presets,
                          QWidget                   *parent = nullptr);

  // Called externally when the attribute's preset list changes.
  void set_presets(const std::vector<Preset> &presets);

  // Updates the gradient bar visualization from external model changes.
  void update_bar();

Q_SIGNALS:
  void value_changed(); // every incremental edit
  void edit_ended(); // committed (drag release, colour picked, preset applied)

protected:
  void  resizeEvent(QResizeEvent *e) override;
  bool  eventFilter(QObject *watched, QEvent *event) override;
  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

private:
  void rebuild_preset_grid();
  void update_content_height();

  std::vector<Stop>  &stops_;
  std::vector<Preset> presets_;

  QScrollArea       *scroll_area_ = nullptr;
  GradientBarWidget *bar_widget_ = nullptr;
  PresetGridWidget  *preset_grid_ = nullptr;
  QWidget           *content_widget_ = nullptr;

  static constexpr int SWATCH_W = 60; // each preset swatch width
  static constexpr int SWATCH_H = 32; // each preset swatch height
};

} // namespace meta::qt