/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <functional>

#include <QLabel>
#include <QWidget>

#include "meta/core/abstract_attribute.hpp"
#include "meta/core/event.hpp"

namespace meta::qt
{

/** @brief Base widget used in the Meta Qt system.
 *
 * MetaWidget provides a common interface for attribute editor widgets. The
 * actual synchronization logic depends on the underlying attribute and widget
 * types and is assigned when the widget is created.
 */
class MetaWidget : public QWidget
{
  Q_OBJECT

public:
  // Make sure the connection is declared after anything it references => the
  // connection must be destroyed before anything else disappears.
  EventConnection connection_;

  /// Construct a MetaWidget.
  MetaWidget(QWidget *parent = nullptr);

  /// Check if widget is currently being edited.
  bool is_editing() const;

  /// Get sync-from-model callback.
  const std::function<void()> &get_sync_from_model() const;

  /** @brief Set callback used to sync widget from model.
   *
   * The callback is typically assigned by the widget factory when the
   * MetaWidget is created. /!\ Use QBlocker in the callback to avoid infinite
   * feedback loops.
   */
  void set_sync_from_model(std::function<void()> callback);

  /// Call the sync-from-model callback if it exists.
  void sync_widget_from_model();

signals:
  /// Emitted when widget is closed.
  void closed();

  /// Emitted when editing starts.
  void edit_started();

  /// Emitted when editing ends.
  void edit_ended();

  /// Emitted when value changes.
  void value_changed();

public slots:
  /** @brief Synchronize the widget from the model.
   *
   * This slot simply forwards to sync_widget_from_model(). The actual
   * synchronization logic is implemented by the callback assigned when the
   * widget is created. Using a callback avoids requiring each specialized
   * widget to implement its own Qt slot while still providing a standard slot
   * that can be connected to Qt signals.
   */
  void on_sync_widget_from_model();

protected:
  /// Handle close event.
  void closeEvent(QCloseEvent *event) override;

private:
  bool                  editing_ = false;
  std::function<void()> sync_from_model_;
};

/// Create a MetaWidget arranged in a grid layout.
MetaWidget *make_meta_widget_grid(QWidget *parent = nullptr);

/// Create a MetaWidget arranged in a horizontal layout.
MetaWidget *make_meta_widget_hbox(QWidget *parent = nullptr);

/// Create a MetaWidget arranged in a vertical layout.
MetaWidget *make_meta_widget_vbox(QWidget *parent = nullptr);

/// Create a QLabel showing an error message for an attribute.
QLabel *make_error_widget(const AbstractAttribute *p_attr,
                          const std::string       &msg = "",
                          QWidget                 *parent = nullptr);

} // namespace meta::qt