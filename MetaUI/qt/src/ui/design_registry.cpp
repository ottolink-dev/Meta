/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/ui/design_registry.hpp"

#include <set>

#include "meta/logger.hpp"

namespace meta::qt
{

DesignRegistry &DesignRegistry::instance()
{
  static DesignRegistry registry;
  return registry;
}

void DesignRegistry::add(const std::string &design,
                         std::type_index    type,
                         const std::string &widget_type,
                         RowFactory         factory)
{
  factories_[design][Key{type, widget_type}] = std::move(factory);
}

void DesignRegistry::set_fallback(const std::string &design,
                                  const std::string &fallback)
{
  fallbacks_[design] = fallback;
}

MetaWidget *DesignRegistry::render(AbstractAttribute *p_attr,
                                   const std::string &design,
                                   const RowContext  &ctx,
                                   QWidget           *parent) const
{
  if (!p_attr) return nullptr;

  // meta::common::widget_type() only has an Attribute<T> overload; on an
  // AbstractAttribute the key has to be read directly.
  const std::string widget_type = meta::common::try_get<std::string>(
      *p_attr,
      meta::keys::ui::widget_type,
      "");

  const std::type_index type{p_attr->type()};

  std::set<std::string> visited;
  std::string           current = design;

  while (!current.empty() && visited.insert(current).second)
  {
    auto design_it = factories_.find(current);

    if (design_it != factories_.end())
    {
      const auto &table = design_it->second;

      // A factory may decline (nullptr) -- can_render() said no -- in which
      // case the walk continues rather than stopping at a blank row.
      if (auto it = table.find(Key{type, widget_type}); it != table.end())
        if (MetaWidget *row = it->second(*p_attr, ctx, parent)) return row;

      if (auto it = table.find(Key{type, kAnyWidgetType}); it != table.end())
        if (MetaWidget *row = it->second(*p_attr, ctx, parent)) return row;
    }

    auto fallback_it = fallbacks_.find(current);
    current = fallback_it == fallbacks_.end() ? std::string{}
                                              : fallback_it->second;
  }

  return nullptr;
}

void DesignRegistry::register_section_factory(const std::string &design,
                                              SectionFactory     factory)
{
  section_factories_[design] = std::move(factory);
}

SectionFactory DesignRegistry::section_factory(const std::string &design) const
{
  std::set<std::string> visited;
  std::string           current = design;

  while (!current.empty() && visited.insert(current).second)
  {
    auto it = section_factories_.find(current);
    if (it != section_factories_.end() && it->second) return it->second;

    auto fallback_it = fallbacks_.find(current);
    current = fallback_it == fallbacks_.end() ? std::string{}
                                              : fallback_it->second;
  }

  // default fallback: standard stock collapsible section
  return [](const QString &title) { return new CollapsibleSection(title); };
}

void DesignRegistry::set_theme(const std::string &design,
                               const std::string &theme_name)
{
  themes_[design] = theme_name;
}

const Theme &DesignRegistry::theme(const std::string &design) const
{
  std::set<std::string> visited;
  std::string           current = design;

  while (!current.empty() && visited.insert(current).second)
  {
    auto it = themes_.find(current);
    if (it != themes_.end() && !it->second.empty())
      return ThemeRegistry::instance().get(it->second);

    auto fallback_it = fallbacks_.find(current);
    current = fallback_it == fallbacks_.end() ? std::string{}
                                              : fallback_it->second;
  }

  return ThemeRegistry::instance().fallback();
}

bool DesignRegistry::has_control(const std::string &design,
                                 std::type_index    type,
                                 const std::string &widget_type) const
{
  std::set<std::string> visited;
  std::string           current = design;

  while (!current.empty() && visited.insert(current).second)
  {
    auto design_it = factories_.find(current);
    if (design_it != factories_.end())
    {
      const auto &table = design_it->second;
      if (table.find(Key{type, widget_type}) != table.end() ||
          table.find(Key{type, kAnyWidgetType}) != table.end())
        return true;
    }

    auto fallback_it = fallbacks_.find(current);
    current = fallback_it == fallbacks_.end() ? std::string{}
                                              : fallback_it->second;
  }

  return false;
}

bool DesignRegistry::has_design(const std::string &design) const
{
  return factories_.find(design) != factories_.end() ||
         section_factories_.find(design) != section_factories_.end();
}

std::vector<std::string> DesignRegistry::designs() const
{
  std::vector<std::string> out;
  out.reserve(factories_.size() + section_factories_.size());
  for (const auto &[name, _] : factories_)
    out.push_back(name);
  for (const auto &[name, _] : section_factories_)
    if (std::find(out.begin(), out.end(), name) == out.end())
      out.push_back(name);
  return out;
}

MetaWidget *render_row(AbstractAttribute *p_attr,
                       const std::string &design,
                       const RowContext  &ctx,
                       QWidget           *parent)
{
  if (!p_attr)
  {
    Logger::log()->error("render_row: incoming p_attr is nullptr");
    return nullptr;
  }

  MetaWidget *row = DesignRegistry::instance().render(p_attr,
                                                      design,
                                                      ctx,
                                                      parent);

  if (!row)
    Logger::log()->error("render_row: no factory in design '{}' (or its "
                         "fallbacks) for attribute "
                         "'{}' of type '{}'",
                         design,
                         p_attr->name(),
                         p_attr->type().name());

  return row;
}

} // namespace meta::qt
