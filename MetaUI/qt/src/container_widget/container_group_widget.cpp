/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <QVBoxLayout>

#include "meta/logger.hpp"

#include "meta_qt/container_group_widget.hpp"
#include "meta_qt/container_widget.hpp"

namespace meta::qt
{

ContainerGroupWidget::ContainerGroupWidget(meta::ContainerGroup  &group,
                                           ContainerRenderOptions options,
                                           QWidget               *parent)
    : MetaWidget(parent), group(group), options(options)
{
  Logger::log()->trace("ContainerGroupWidget::ContainerGroupWidget");

  auto *root = new QVBoxLayout(this);
  this->setLayout(root);

  // --- Selector

  combo = new QComboBox(this);
  root->addWidget(combo);

  stacked = new QStackedWidget(this);
  root->addWidget(stacked);

  // fill containers
  for (const auto &key : group.insertion_order())
  {
    Logger::log()->trace(
        "ContainerGroupWidget::ContainerGroupWidget: adding page '{}'",
        key);

    combo->addItem(QString::fromStdString(key));

    // build widget ONCE
    auto *w = build_container_widget(key);
    pages[key] = w;
    stacked->addWidget(w);
  }

  // set initial
  sync_stack();

  // --- Switching

  connect(combo,
          &QComboBox::currentTextChanged,
          this,
          [this](const QString &text)
          {
            const std::string new_current = text.toStdString();

            Logger::log()->trace("ContainerGroupWidget: switching to '{}'",
                                 new_current);

            this->group.set_current(new_current);
            sync_stack();

            Q_EMIT current_container_changed(new_current);
          });

  // pass through for the synching from the model (the
  // ContainerGroupWidget is derived from a MetaWidget)
  set_sync_from_model([this]() { this->on_sync_meta_widgets_from_model(); });
}

QWidget *ContainerGroupWidget::build_container_widget(const std::string &key)
{
  Logger::log()->trace("ContainerGroupWidget::build_container_widget: '{}'",
                       key);

  auto *container = group.find(key);

  if (!container)
  {
    Logger::log()->warn("ContainerGroupWidget::build_container_widget: "
                        "container '{}' not found",
                        key);

    return new QWidget(); // dummy
  }

  MetaWidget *container_widget = meta::qt::render(*container, options);

  // pass-through signals
  connect(container_widget,
          &MetaWidget::edit_started,
          this,
          &MetaWidget::edit_started);

  connect(container_widget,
          &MetaWidget::value_changed,
          this,
          &MetaWidget::value_changed);

  connect(container_widget,
          &MetaWidget::edit_ended,
          this,
          &MetaWidget::edit_ended);

  return container_widget;
}

void ContainerGroupWidget::on_sync_meta_widgets_from_model()
{
  Logger::log()->trace("ContainerGroupWidget::on_sync_meta_widgets_from_model");

  for (int i = 0; i < stacked->count(); ++i)
  {
    // those MetaWidgets are container widgets, passing the sync to
    // the individual MetaWidgets is carried out by the container
    // widget
    if (auto *meta_widget = qobject_cast<MetaWidget *>(stacked->widget(i)))
      meta_widget->sync_widget_from_model();
  }
}

void ContainerGroupWidget::sync_stack()
{
  Logger::log()->trace("ContainerGroupWidget::sync_stack");

  std::optional<std::string> current_name = group.current_container_name();

  if (!current_name)
  {
    Logger::log()->trace(
        "ContainerGroupWidget::sync_stack: no current container");

    return;
  }

  int index = combo->findText(QString::fromStdString(*current_name));

  if (index >= 0)
  {
    Logger::log()->trace(
        "ContainerGroupWidget::sync_stack: current='{}' index={}",
        *current_name,
        index);

    stacked->setCurrentIndex(index);
    combo->setCurrentIndex(index);
  }
  else
  {
    Logger::log()->warn(
        "ContainerGroupWidget::sync_stack: container '{}' not found in combo",
        *current_name);
  }
}

// --- Function

MetaWidget *render(meta::ContainerGroup  &group,
                   ContainerRenderOptions options,
                   QWidget               *parent,
                   bool                   render_single_group_as_a_container)
{
  Logger::log()->trace("ContainerGroupWidget::render");

  if (group.size() == 0)
  {
    Logger::log()->error("render / meta::ContainerGroup: empty group");
    return nullptr;
  }

  // group with 1 container => flatten to a single attribute container
  // if requested
  if (render_single_group_as_a_container && group.size() == 1)
  {
    auto &containers = group.containers();
    return render(*containers.begin()->second, options, parent);
  }

  // default
  return new ContainerGroupWidget(group, options, parent);
}

} // namespace meta::qt
