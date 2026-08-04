/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <optional>
#include <regex>

#include <QString>

#include "meta/logger.hpp"
#include "meta_common.hpp"

#include "meta_qt/container_widget.hpp"
#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widget_renderer.hpp"
#include "meta_qt/widgets/collapsible_section.hpp"
#include "meta_qt/widgets/preset_combo_box.hpp"

namespace meta::qt
{

std::string compute_flattened_path(CategoryNode *node)
{
  std::string path;

  while (node)
  {
    if (!path.empty() && !node->name.empty()) path += "/";

    path += node->name;

    if (node->attributes.size() > 0) break;
    if (node->children.size() != 1) break;

    node = node->children.begin()->second.get();
  }

  return path;
}

void insert_attribute(CategoryNode      &root,
                      const std::string &path,
                      AbstractAttribute *attr)
{
  CategoryNode *node = &root;

  std::stringstream ss(path);
  std::string       part;

  while (std::getline(ss, part, '/'))
  {
    auto &child = node->children[part];

    if (!child)
    {
      child = std::make_unique<CategoryNode>();
      child->name = part;
    }

    node = child.get();
  }

  node->attributes.push_back(attr);
}

void render_flat(CategoryNode              &node,
                 QVBoxLayout               *layout,
                 std::vector<MetaWidget *> &collected_widgets)
{
  Logger::log()->trace("container_widget::render_flat");

  for (auto *p_attr : node.attributes)
  {
    Logger::log()->trace("container_widget::render_flat: '{}'",
                         p_attr ? p_attr->name() : std::string("null"));

    MetaWidget *w = meta::qt::render(p_attr);

    if (w)
    {
      if (const std::string tip = meta::common::try_get<std::string>(
              *p_attr,
              meta::keys::ui::tooltip,
              "");
          !tip.empty())
        w->setToolTip(QString::fromStdString(tip));
    }

    layout->addWidget(w);
    collected_widgets.push_back(w);
  }

  for (auto &[name, child] : node.children)
    render_flat(*child, layout, collected_widgets);
}

void render_category(meta::AttributeContainer  &container,
                     CategoryNode              &node,
                     QVBoxLayout               *parent_layout,
                     std::vector<MetaWidget *> &collected_widgets)
{
  Logger::log()->trace("container_widget::render_category: '{}'", node.name);

  QVBoxLayout *current_layout = parent_layout;

  if (!node.name.empty())
  {
    const std::string title = node.name;

    auto *section = new CollapsibleSection(title.c_str());
    parent_layout->addWidget(section);

    Logger::log()->trace("container_widget::render_category: section '{}'",
                         title);

    // UI state management
    {
      auto *state = container.try_add(meta::keys::ui::state, true);
      state->metadata().try_add(meta::keys::ui::widget_type, "None");

      // apply stored state if available
      if (state->metadata().contains(title))
      {
        bool current_state = state->metadata().value<bool>(title);
        section->set_expanded(current_state);
      }

      QObject::connect(
          section,
          &CollapsibleSection::expanded_state_changed,
          [&container, title](bool new_state)
          {
            // add or get existing (dummy attribute used as a
            // metadata container)
            auto *state = container.try_add(meta::keys::ui::state, true);
            state->metadata().try_add(title, new_state)->value() = new_state;
          });
    }

    current_layout = section->content_layout;
  }

  for (auto *p_attr : node.attributes)
  {
    Logger::log()->trace("container_widget::render_category: '{}'",
                         p_attr ? p_attr->name() : std::string("null"));

    MetaWidget *w = meta::qt::render(p_attr);

    if (w) // avoid 'None' type widgets
    {
      if (const std::string tip = meta::common::try_get<std::string>(
              *p_attr,
              meta::keys::ui::tooltip,
              "");
          !tip.empty())
        w->setToolTip(QString::fromStdString(tip));

      current_layout->addWidget(w);
      collected_widgets.push_back(w);
    }
  }

  for (auto &[name, child] : node.children)
    render_category(container, *child, current_layout, collected_widgets);
}

void render_category_merged(meta::AttributeContainer        &container,
                            CategoryNode                    &node,
                            QVBoxLayout                     *parent_layout,
                            std::vector<MetaWidget *>       &collected_widgets,
                            const std::optional<std::regex> &collapse_regex)
{
  Logger::log()->trace("container_widget::render_category_merged");

  CategoryNode               *current = &node;
  std::vector<CategoryNode *> chain;

  while (current)
  {
    chain.push_back(current);
    if (current->children.size() != 1) break;
    current = current->children.begin()->second.get();
  }

  std::string title;
  for (auto *n : chain)
  {
    if (!n->name.empty())
    {
      if (!title.empty()) title += "/";
      title += n->name;
    }
  }

  auto *section = new CollapsibleSection(title.c_str());

  if (collapse_regex && std::regex_search(title, *collapse_regex))
  {
    Logger::log()->trace(
        "container_widget::render_category_merged: auto-collapse '{}'",
        title);

    section->set_expanded(false);
  }

  // UI state management
  {
    auto *state = container.try_add(meta::keys::ui::state, true);
    state->metadata().try_add(meta::keys::ui::widget_type, "None");

    // apply stored state if available
    if (state->metadata().contains(title))
    {
      bool current_state = state->metadata().value<bool>(title);
      section->set_expanded(current_state);
    }

    QObject::connect(
        section,
        &CollapsibleSection::expanded_state_changed,
        [&container, title](bool new_state)
        {
          // add or get existing (dummy attribute used as a
          // metadata container)
          auto *state = container.try_add(meta::keys::ui::state, true);
          state->metadata().try_add(title, new_state)->value() = new_state;
        });
  }

  parent_layout->addWidget(section);

  QVBoxLayout *layout = section->content_layout;

  for (auto *n : chain)
    for (auto *p_attr : n->attributes)
    {
      Logger::log()->trace("container_widget::render_category_merged: '{}'",
                           p_attr ? p_attr->name() : std::string("null"));

      MetaWidget *w = meta::qt::render(p_attr);

      if (w) // 'None' widget is possible
      {
        if (const std::string tip = meta::common::try_get<std::string>(
                *p_attr,
                meta::keys::ui::tooltip,
                "");
            !tip.empty())
          w->setToolTip(QString::fromStdString(tip));

        layout->addWidget(w);
        collected_widgets.push_back(w);
      }
    }

  CategoryNode *last = chain.back();

  for (auto &[name, child] : last->children)
    render_category_merged(container,
                           *child,
                           layout,
                           collected_widgets,
                           collapse_regex);
}

MetaWidget *render(AttributeContainer    &container,
                   ContainerRenderOptions options,
                   QWidget               *parent)
{
  Logger::log()->trace("container_widget::render");

  CategoryNode root;

  if (options.root_category_name.empty()) root.name = META_ROOT_CATEGORY;

  bool has_no_categorys = true;

  const std::vector<std::string> &order = options.insertion_order.empty()
                                              ? container.insertion_order()
                                              : options.insertion_order;

  Logger::log()->trace("container_widget::render: {} attributes", order.size());

  for (const auto &name : order)
  {
    auto *attr = container.find(name);

    if (!attr)
    {
      Logger::log()->error(
          "render: attribute '{}' not found in container, skipping",
          name);
      continue;
    }

    const std::string category = meta::common::category(*attr);
    insert_attribute(root, category, attr);
    has_no_categorys &= category.empty();

    Logger::log()->trace("container_widget::render: '{}', category: '{}'",
                         name,
                         category);
  }

  MetaWidget *container_widget = make_meta_widget_vbox(parent);
  auto       *layout = static_cast<QVBoxLayout *>(container_widget->layout());

  // --- Snapshots

  PresetComboBox *presets = nullptr;

  if (options.snapshot_manager)
  {
    Logger::log()->trace("container_widget::render: enabling presets");

    presets = new PresetComboBox(&container.snapshot_manager());
    layout->addWidget(presets);

    // define save snapshot function
    presets->set_snapshot_provider([&container]()
                                   { return container.json_to(); });
  }

  // --- Attribute widgets

  std::vector<MetaWidget *> collected_widgets;

  switch (options.category_policy)
  {
  case CategoryPolicy::CP_TREE:
    Logger::log()->trace("container_widget::render: tree mode");
    render_category(container, root, layout, collected_widgets);
    break;

  case CategoryPolicy::CP_MERGED:
    Logger::log()->trace("container_widget::render: merged mode");
    render_category_merged(container,
                           root,
                           layout,
                           collected_widgets,
                           options.collapse_regex);
    break;

  case CategoryPolicy::CP_SMART:
    Logger::log()->trace("container_widget::render: smart mode");

    if (has_no_categorys)
      render_flat(root, layout, collected_widgets);
    else
      render_category_merged(container,
                             root,
                             layout,
                             collected_widgets,
                             options.collapse_regex);
    break;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

  case CategoryPolicy::CP_FLAT:
    Logger::log()->trace("container_widget::render: flat mode");
  default: render_flat(root, layout, collected_widgets); break;

#pragma GCC diagnostic pop
  }

  for (MetaWidget *w : collected_widgets)
  {
    QObject::connect(w,
                     &MetaWidget::edit_started,
                     container_widget,
                     &MetaWidget::edit_started);

    QObject::connect(w,
                     &MetaWidget::value_changed,
                     container_widget,
                     &MetaWidget::value_changed);

    QObject::connect(w,
                     &MetaWidget::edit_ended,
                     container_widget,
                     &MetaWidget::edit_ended);
  }

  // chain the container update request with the collected widget syncing
  container_widget->set_sync_from_model(
      [collected_widgets]()
      {
        for (auto w : collected_widgets)
          w->sync_widget_from_model();
      });

  Logger::log()->trace("container_widget::render: {} widgets created",
                       collected_widgets.size());

  if (options.snapshot_manager)
  {
    Logger::log()->trace("container_widget::render: presets connection");

    QObject::connect(
        presets,
        &PresetComboBox::preset_selected,
        container_widget,
        [&container,
         collected_widgets,
         container_widget](std::string /* name */, nlohmann::json snapshot)
        {
          // update model first (do not overwrite the snapshot data)
          bool exclude_snapshot_manager = true;
          container.json_from(snapshot, exclude_snapshot_manager);

          // sync all widgets
          for (MetaWidget *w : collected_widgets)
          {
            const QSignalBlocker blocker(w);
            w->sync_widget_from_model();
          }

          Q_EMIT container_widget->value_changed();
          Q_EMIT container_widget->edit_ended();
        },
        Qt::DirectConnection);
  }

  return container_widget;
}

} // namespace meta::qt
