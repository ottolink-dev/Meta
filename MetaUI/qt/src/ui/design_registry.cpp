/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/ui/design_registry.hpp"

#include "meta/logger.hpp"

#include "meta_qt/widget_renderer.hpp"

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

MetaWidget *DesignRegistry::render(AbstractAttribute *p_attr,
                                   const std::string &design,
                                   const RowContext  &ctx,
                                   QWidget           *parent) const
{
  if (!p_attr) return nullptr;

  auto design_it = factories_.find(design);
  if (design_it == factories_.end()) return nullptr;

  const auto &table = design_it->second;

  // meta::common::widget_type() only has an Attribute<T> overload; on an
  // AbstractAttribute the key has to be read directly.
  const std::string widget_type = meta::common::try_get<std::string>(
      *p_attr,
      meta::keys::ui::widget_type,
      "");

  const std::type_index type{p_attr->type()};

  if (auto it = table.find(Key{type, widget_type}); it != table.end())
    return it->second(*p_attr, ctx, parent);

  if (auto it = table.find(Key{type, kAnyWidgetType}); it != table.end())
    return it->second(*p_attr, ctx, parent);

  return nullptr;
}

bool DesignRegistry::has_design(const std::string &design) const
{
  return factories_.find(design) != factories_.end();
}

std::vector<std::string> DesignRegistry::designs() const
{
  std::vector<std::string> out;
  out.reserve(factories_.size());
  for (const auto &[name, _] : factories_)
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

  if (MetaWidget *row = DesignRegistry::instance().render(p_attr, design, ctx, parent))
    return row;

  // Either nothing is registered for this (design, type, widget_type), or the
  // registered control declined the attribute via can_render(). The stock
  // renderer covers every type Meta supports, so an unported -- or unrenderable
  // -- attribute degrades to a plain widget rather than leaving a gap.
  return render(p_attr, parent);
}

} // namespace meta::qt
