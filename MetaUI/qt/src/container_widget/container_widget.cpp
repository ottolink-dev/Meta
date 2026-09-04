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
#include "meta_qt/ui/design_registry.hpp"
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
      node->children_order.push_back(part);
    }

    node = child.get();
  }

  node->attributes.push_back(attr);
}

namespace
{

/// The stock section unless the caller supplied a design-aware factory.
CollapsibleSection *build_section(const SectionFactory &section_factory,
                                  const QString        &title)
{
  return section_factory ? section_factory(title)
                         : new CollapsibleSection(title);
}

/// The stock renderer unless the caller supplied a design-aware one.
MetaWidget *build_row(const AttributeRowRenderer &row_renderer,
                      AbstractAttribute          *p_attr)
{
  return row_renderer ? row_renderer(p_attr) : qt::render(p_attr);
}

} // namespace

void render_flat(CategoryNode               &node,
                 QVBoxLayout                *layout,
                 std::vector<MetaWidget *>  &collected_widgets,
                 const AttributeRowRenderer &row_renderer)
{
  Logger::log()->trace("container_widget::render_flat");

  for (auto *p_attr : node.attributes)
  {
    Logger::log()->trace("container_widget::render_flat: '{}'",
                         p_attr ? p_attr->name() : std::string("null"));

    MetaWidget *w = build_row(row_renderer, p_attr);

    if (w)
    {
      if (const std::string tip = common::try_get<std::string>(
              *p_attr,
              keys::ui::tooltip,
              "");
          !tip.empty())
        w->setToolTip(QString::fromStdString(tip));
    }

    layout->addWidget(w);
    collected_widgets.push_back(w);
  }

  for (const auto &name : node.children_order)
    render_flat(*node.children.at(name),
                layout,
                collected_widgets,
                row_renderer);
}

void render_category(AttributeContainer        &container,
                     CategoryNode              &node,
                     QVBoxLayout               *parent_layout,
                     std::vector<MetaWidget *> &collected_widgets,
                     std::vector<std::pair<CollapsibleSection *, std::string>>
                                                &collected_sections,
                     const AttributeRowRenderer &row_renderer,
                     const SectionFactory       &section_factory)
{
  Logger::log()->trace("container_widget::render_category: '{}'", node.name);

  QVBoxLayout *current_layout = parent_layout;

  if (!node.name.empty())
  {
    const std::string title = node.name;

    auto *section = build_section(section_factory, title.c_str());
    parent_layout->addWidget(section);

    Logger::log()->trace("container_widget::render_category: section '{}'",
                         title);

    // UI state management
    {
      const std::string is_expanded_key = title + ".is_expanded";

      container.state().try_add<bool>(is_expanded_key, false);

      bool current_state = container.state().value<bool>(is_expanded_key);
      section->set_expanded(current_state);

      collected_sections.emplace_back(section, is_expanded_key);

      QObject::connect(section,
                       &CollapsibleSection::expanded_state_changed,
                       [&container, is_expanded_key](bool new_state)
                       {
                         container.state()
                             .try_add(is_expanded_key, new_state)
                             ->value() = new_state;
                       });
    }

    current_layout = section->content_layout;
  }

  for (auto *p_attr : node.attributes)
  {
    Logger::log()->trace("container_widget::render_category: '{}'",
                         p_attr ? p_attr->name() : std::string("null"));

    MetaWidget *w = build_row(row_renderer, p_attr);

    if (w) // avoid 'None' type widgets
    {
      if (const std::string tip = common::try_get<std::string>(
              *p_attr,
              keys::ui::tooltip,
              "");
          !tip.empty())
        w->setToolTip(QString::fromStdString(tip));

      current_layout->addWidget(w);
      collected_widgets.push_back(w);
    }
  }

  for (const auto &name : node.children_order)
    render_category(container,
                    *node.children.at(name),
                    current_layout,
                    collected_widgets,
                    collected_sections,
                    row_renderer,
                    section_factory);
}

void render_category_merged(
    AttributeContainer        &container,
    CategoryNode              &node,
    QVBoxLayout               *parent_layout,
    std::vector<MetaWidget *> &collected_widgets,
    std::vector<std::pair<CollapsibleSection *, std::string>>
                                    &collected_sections,
    const std::optional<std::regex> &collapse_regex,
    const AttributeRowRenderer      &row_renderer,
    const SectionFactory            &section_factory)
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

  QVBoxLayout *layout = parent_layout;

  if (!title.empty())
  {
    auto *section = build_section(section_factory, title.c_str());

    const bool autocollapse = collapse_regex &&
                              std::regex_search(title, *collapse_regex);
    const bool default_expansion_value = !autocollapse;

    // UI state management
    {
      const std::string is_expanded_key = title + ".is_expanded";

      container.state().try_add<bool>(is_expanded_key,
                                      bool(default_expansion_value));

      bool current_state = container.state().value<bool>(is_expanded_key);
      section->set_expanded(current_state);

      collected_sections.emplace_back(section, is_expanded_key);

      QObject::connect(section,
                       &CollapsibleSection::expanded_state_changed,
                       [&container, is_expanded_key](bool new_state)
                       {
                         container.state()
                             .try_add(is_expanded_key, new_state)
                             ->value() = new_state;
                       });
    }

    parent_layout->addWidget(section);
    layout = section->content_layout;
  }

  for (auto *n : chain)
    for (auto *p_attr : n->attributes)
    {
      Logger::log()->trace("container_widget::render_category_merged: '{}'",
                           p_attr ? p_attr->name() : std::string("null"));

      MetaWidget *w = build_row(row_renderer, p_attr);

      if (w) // 'None' widget is possible
      {
        if (const std::string tip = common::try_get<std::string>(
                *p_attr,
                keys::ui::tooltip,
                "");
            !tip.empty())
          w->setToolTip(QString::fromStdString(tip));

        layout->addWidget(w);
        collected_widgets.push_back(w);
      }
    }

  CategoryNode *last = chain.back();

  for (const auto &name : last->children_order)
    render_category_merged(container,
                           *last->children.at(name),
                           layout,
                           collected_widgets,
                           collected_sections,
                           collapse_regex,
                           row_renderer,
                           section_factory);
}

MetaWidget *render(AttributeContainer    &container,
                   ContainerRenderOptions options,
                   QWidget               *parent)
{
  Logger::log()->trace("container_widget::render");

  CategoryNode root;
  root.name = options.root_category_name;

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

    const std::string category = common::category(*attr);
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

  AttributeRowRenderer effective_row_renderer = options.row_renderer;
  if (!effective_row_renderer)
  {
    effective_row_renderer = [design = options.design,
                              ctx = options.row_context,
                              parent](AbstractAttribute *p_attr) -> MetaWidget *
    { return render_row(p_attr, design, ctx, parent); };
  }

  SectionFactory effective_section_factory = options.section_factory;
  if (!effective_section_factory)
  {
    effective_section_factory = DesignRegistry::instance().section_factory(
        options.design);
  }

  std::vector<MetaWidget *>                                 collected_widgets;
  std::vector<std::pair<CollapsibleSection *, std::string>> collected_sections;

  switch (options.category_policy)
  {
  case CategoryPolicy::CP_TREE:
    Logger::log()->trace("container_widget::render: tree mode");
    render_category(container,
                    root,
                    layout,
                    collected_widgets,
                    collected_sections,
                    effective_row_renderer,
                    effective_section_factory);
    break;

  case CategoryPolicy::CP_MERGED:
    Logger::log()->trace("container_widget::render: merged mode");
    render_category_merged(container,
                           root,
                           layout,
                           collected_widgets,
                           collected_sections,
                           options.collapse_regex,
                           effective_row_renderer,
                           effective_section_factory);
    break;

  case CategoryPolicy::CP_SMART:
    Logger::log()->trace("container_widget::render: smart mode");

    if (has_no_categorys)
      render_flat(root, layout, collected_widgets, effective_row_renderer);
    else
      render_category_merged(container,
                             root,
                             layout,
                             collected_widgets,
                             collected_sections,
                             options.collapse_regex,
                             effective_row_renderer,
                             effective_section_factory);
    break;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

  case CategoryPolicy::CP_FLAT:
    Logger::log()->trace("container_widget::render: flat mode");
  default:
    render_flat(root, layout, collected_widgets, effective_row_renderer);
    break;

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
      [&container, collected_widgets, collected_sections]()
      {
        // underlying widgets
        for (auto w : collected_widgets)
          w->sync_widget_from_model();

        // collapsible section state
        for (const auto &[section, is_expanded_key] : collected_sections)
        {
          if (container.state().contains(is_expanded_key))
          {
            const bool is_expanded = container.state().value<bool>(
                is_expanded_key);
            section->set_expanded(is_expanded);
          }
        }
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
        [&container, collected_widgets, collected_sections, container_widget](
            std::string /* name */,
            nlohmann::json snapshot)
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

          for (const auto &[section, is_expanded_key] : collected_sections)
          {
            if (container.state().contains(is_expanded_key))
            {
              const bool is_expanded = container.state().value<bool>(
                  is_expanded_key);
              section->set_expanded(is_expanded);
            }
          }

          Q_EMIT container_widget->value_changed();
          Q_EMIT container_widget->edit_ended();
        },
        Qt::DirectConnection);
  }

  return container_widget;
}

} // namespace meta::qt
