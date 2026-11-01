/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <functional>
#include <string>

#include <QComboBox>
#include <QWidget>

#include <nlohmann/json.hpp>

#include "meta/core/event.hpp"
#include "meta/serialization/snapshot_manager.hpp"
#include "meta_qt/ui/theme.hpp"

namespace meta::qt
{

/**
 * @brief Combo box backed by a SnapshotManager: offers a "Save preset..."
 * action item followed by the list of saved preset names ("default" pinned
 * first if present, remaining names sorted alphabetically).
 * Supports optional industrial styling via set_theme().
 */
class PresetComboBox : public QWidget
{
  Q_OBJECT

public:
  EventConnection connection_;

  /// Ctor
  explicit PresetComboBox(SnapshotManager *snapshot_manager,
                          QWidget         *parent = nullptr);

  /// Apply a Theme to enable custom industrial styling with card margin
  void set_theme(const Theme &theme);

  /**
   * @brief Set the callback invoked to capture the current state when the
   * user saves a new preset. Must be set before a save can succeed.
   */
  void set_snapshot_provider(std::function<nlohmann::json()> provider);

  /// Re-read the preset list from the snapshot manager and rebuild the combo.
  void refresh();

  /// Programmatically select a preset by name, if it exists.
  void set_current_preset(const std::string &name);

  /// Name of the currently selected preset (empty if none / "Save preset..."
  /// shown).
  std::string get_current_preset() const;

signals:
  /// Emitted when the user picks an existing preset from the list.
  void preset_selected(std::string name, nlohmann::json snapshot);

  /// Emitted after a new preset has been successfully saved.
  void preset_saved(std::string name);

  /// Emitted after a preset has been deleted.
  void preset_deleted(std::string name);

protected:
  void  mouseReleaseEvent(QMouseEvent *event) override;
  void  paintEvent(QPaintEvent *event) override;
  QSize sizeHint() const override;

private slots:
  void on_context_menu_requested(const QPoint &pos);
  void on_index_activated(int index);

private:
  void delete_preset(const std::string &name);
  void open_industrial_popup();
  void save_new_preset();

  void populate_combo(const std::string &select_name = {});

  SnapshotManager                *snapshot_manager = nullptr;
  QComboBox                      *combo = nullptr;
  std::function<nlohmann::json()> snapshot_provider;
  std::string                     current_preset;

  bool  industrial_ = false;
  Theme theme_;
  bool  popup_open_ = false;

  static constexpr int save_action_index = 0;
};

} // namespace meta::qt