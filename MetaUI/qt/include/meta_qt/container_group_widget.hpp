/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <regex>

#include <QComboBox>
#include <QStackedWidget>
#include <QTabWidget>

#include "meta/core/container_group.hpp"

#include "meta_qt/container_widget.hpp"
#include "meta_qt/meta_widget.hpp"

namespace meta::qt
{

/// Widget that displays and edits a ContainerGroup using a stacked UI or tabs.
class ContainerGroupWidget : public MetaWidget
{
  Q_OBJECT

public:
  /// Construct a ContainerGroupWidget.
  ContainerGroupWidget(
      meta::ContainerGroup  &group,
      ContainerRenderOptions options = ContainerRenderOptions{},
      QWidget               *parent = nullptr);

public slots:
  /// Synchronize the contained MetaWidgets from their model.
  void on_sync_meta_widgets_from_model();

signals:
  /// User has selected another container.
  void current_container_changed(const std::string name);

private:
  /// Build a widget for a single container entry.
  QWidget *build_container_widget(const std::string &key);

  /// Synchronize tab or combo box selection with stacked widget page.
  void sync_stack();

private:
  meta::ContainerGroup  &group;   /// Underlying container group
  ContainerRenderOptions options; /// Rendering options

  QTabWidget *tabs = nullptr; /// Tab widget (when using GSM_TABS)
  QComboBox  *combo =
      nullptr; /// Selector for container keys (when using GSM_COMBO_BOX)
  QStackedWidget *stacked =
      nullptr; /// Stacked pages for each container (when using GSM_COMBO_BOX)

  std::unordered_map<std::string, QWidget *> pages; /// Cached page widgets
};

/// Render a ContainerGroup into a MetaWidget.
MetaWidget *render(meta::ContainerGroup  &group,
                   ContainerRenderOptions options = ContainerRenderOptions{},
                   QWidget               *parent = nullptr,
                   bool render_single_group_as_a_container = false);

} // namespace meta::qt