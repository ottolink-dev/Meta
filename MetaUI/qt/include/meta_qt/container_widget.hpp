/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <regex>

#include <QVBoxLayout>

#include "meta/core/abstract_attribute.hpp"
#include "meta/serialization/snapshot_manager.hpp"
#include "meta_common.hpp"

#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widgets/collapsible_section.hpp"

namespace meta::qt
{

/// Policy used to organize attribute categories in the UI.
enum CategoryPolicy
{
  CP_FLAT,   ///< No hierarchy, all attributes in a single list
  CP_TREE,   ///< Strict tree hierarchy based on attribute paths
  CP_MERGED, ///< Merge similar categories into shared groups
  CP_SMART   ///< Heuristic-based hybrid organization
};

/// Policy used to switch between groups in a ContainerGroup.
enum GroupSwitchMode
{
  GSM_TABS,     ///< Use tabs for switching groups
  GSM_COMBO_BOX ///< Use a combo box for switching groups
};

/** @brief Builds the widget for a single attribute.
 *
 * The indirection that lets a host supply an alternative widget design without
 * the container layer knowing any design exists. Leave unset for the stock
 * renderer; see meta_qt/ui/design_registry.hpp for the registry-backed one.
 */
using AttributeRowRenderer = std::function<MetaWidget *(AbstractAttribute *)>;

/** @brief Builds the collapsible section used for a category.
 *
 * Same indirection as AttributeRowRenderer, for the chrome around the rows
 * rather than the rows themselves. A section is not bound to an attribute, so
 * it cannot go through the design registry; it is supplied here instead.
 * Leave unset for the stock section.
 */
using SectionFactory = std::function<CollapsibleSection *(const QString &title)>;

/// Options controlling how attribute containers are rendered.
struct ContainerRenderOptions
{
  // clang-format off
  CategoryPolicy category_policy = CategoryPolicy::CP_SMART;     ///< Category organization strategy
  GroupSwitchMode group_switch_mode = GroupSwitchMode::GSM_TABS; ///< Container group switching style
  std::string root_category_name = META_ROOT_CATEGORY;           ///< Optional root category label
  std::vector<std::string> insertion_order = {};                 ///< Explicit ordering of categories
  std::optional<std::regex> collapse_regex = std::nullopt;       ///< Regex used to collapse categories
  bool snapshot_manager = false;                                 ///< Add snapshot manager widget
  AttributeRowRenderer row_renderer = {};                        ///< Per-attribute widget builder; empty = stock
  SectionFactory section_factory = {};                           ///< Category section builder; empty = stock
  // clang-format on
};

/// Node representing a category in the attribute hierarchy.
struct CategoryNode
{
  // clang-format off
  std::string name;                                              ///< Category name
  std::vector<meta::AbstractAttribute *> attributes;             ///< Attributes in this category
  std::map<std::string, std::unique_ptr<CategoryNode>> children; ///< Subcategories
  std::vector<std::string> children_order;                      ///< Insertion order of subcategories
  // clang-format on
};

/// Inserts an attribute into the category tree using a hierarchical path.
void insert_attribute(CategoryNode            &root,
                      const std::string       &path,
                      meta::AbstractAttribute *p_attr);

/// Computes a flattened string path for a category node.
std::string compute_flattened_path(CategoryNode *node);

/// Renders a flat list of attributes into a Qt layout.
void render_flat(CategoryNode               &node,
                 QVBoxLayout                *layout,
                 std::vector<MetaWidget *>  &collected_widgets,
                 const AttributeRowRenderer &row_renderer = {});

/// Renders a category tree using hierarchical grouping.
void render_category(meta::AttributeContainer  &container,
                     CategoryNode              &node,
                     QVBoxLayout               *parent_layout,
                     std::vector<MetaWidget *> &collected_widgets);

/// Renders attributes grouped into merged categories.
void render_group_merged(meta::AttributeContainer  &container,
                         CategoryNode              &node,
                         QVBoxLayout               *parent_layout,
                         std::vector<MetaWidget *> &collected_widgets);

/// Main entry point for rendering an attribute container into widgets.
MetaWidget *render(meta::AttributeContainer &container,
                   ContainerRenderOptions    options = ContainerRenderOptions{},
                   QWidget                  *parent = nullptr);

} // namespace meta::qt