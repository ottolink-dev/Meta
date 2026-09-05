/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <array>
#include <string>
#include <vector>

#include <QWidget>

#include "meta/core/event.hpp"
#include "meta/ext/color_gradient/color_gradient.hpp"

class QComboBox;
class QPixmap;
class QScrollArea;
class QToolButton;

namespace meta::qt {

class PresetGridWidget;

// ---------------------------------------------------------------------------
// GradientBarWidget
//
// Custom-painted gradient bar with interactive stop handles.
// ---------------------------------------------------------------------------

class GradientBarWidget : public QWidget {
  Q_OBJECT

public:
  static constexpr int BAR_H = 28;
  static constexpr int STOP_R = 6;
  static constexpr int PAD = 10;
  static constexpr int TOTAL_H = PAD + BAR_H + STOP_R * 2 + 6;
  static constexpr int RADIUS = 4;

  explicit GradientBarWidget(std::vector<Stop> &stops,
                             QWidget *parent = nullptr);

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
  int hit_test(const QPoint &pos) const;

  std::vector<Stop> &stops_;
  int selected_idx_ = -1;
  bool dragging_ = false;
};

// ---------------------------------------------------------------------------
// GradientPicker
//
// A self-contained gradient editor widget.
//
// Top:     Gradient bar with draggable stops (double-click to add/edit,
//          drag to move, right-click to remove).
// Middle:  Toolbar: save the current gradient to the user's GradientLibrary,
//          import/export gradient files, choose the preset ordering.
// Bottom:  Vertically scrollable grid of preset swatches merging the host
//          presets (attribute metadata) with the GradientLibrary. Favourites
//          are pinned first; right-click a swatch for favourite / rename /
//          replace / delete / export.
//
// The grid is held within a vertical QScrollArea so that reducing the widget
// height never squashes or clips the gradient visualization.
// ---------------------------------------------------------------------------

class GradientPicker : public QWidget {
  Q_OBJECT

public:
  explicit GradientPicker(std::vector<Stop> &stops,
                          const std::vector<Preset> &presets,
                          QWidget *parent = nullptr);

  // Called externally when the attribute's preset list changes.
  void set_presets(const std::vector<Preset> &presets);

  // Updates the gradient bar visualization from external model changes.
  void update_bar();

  // Stores the current gradient in GradientLibrary::instance() under `name`
  // (made unique against host presets and the library). Returns the stored
  // name.
  std::string save_current_as_preset(const std::string &name);

  // Preset names in display order (favourites first, then the library's sort
  // key). Exposed for tests and hosts.
  std::vector<std::string> entry_names() const;

Q_SIGNALS:
  void value_changed(); // every incremental edit
  void edit_ended(); // committed (drag release, colour picked, preset applied)

protected:
  void resizeEvent(QResizeEvent *e) override;
  bool eventFilter(QObject *watched, QEvent *event) override;
  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

private:
  struct Entry {
    Preset preset;
    bool user = false; // true: GradientLibrary entry, false: host preset
  };

  QWidget *build_toolbar();
  void schedule_rebuild();
  void rebuild_entries();
  void rebuild_preset_grid();
  QPixmap make_swatch(const Entry &entry, bool favorite) const;
  void apply_stops(const std::vector<Stop> &stops);

  std::vector<std::string> host_names() const;

  void on_save_clicked();
  void on_import_clicked();
  void on_export_clicked();
  void show_entry_menu(Entry entry, const QPoint &global_pos);
  void export_presets(const std::vector<Preset> &presets,
                      const QString &suggested_file);

  std::vector<Stop> &stops_;
  std::vector<Preset> presets_; // host presets (attribute metadata)
  std::vector<Entry> entries_;  // host + library, display order

  GradientBarWidget *bar_widget_ = nullptr;
  QToolButton *save_button_ = nullptr;
  QToolButton *import_button_ = nullptr;
  QToolButton *export_button_ = nullptr;
  QComboBox *sort_combo_ = nullptr;
  QScrollArea *scroll_area_ = nullptr;
  PresetGridWidget *preset_grid_ = nullptr;
  bool rebuild_pending_ = false;

  static constexpr int SWATCH_W = 60;  // each preset swatch width
  static constexpr int SWATCH_H = 32;  // each preset swatch height
  static constexpr int TOOLBAR_H = 24; // toolbar row height

  // Declared last so it disconnects before the members its callback touches
  // are destroyed.
  EventConnection library_connection_;
};

} // namespace meta::qt
