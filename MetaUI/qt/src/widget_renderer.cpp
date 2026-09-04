/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/widget_renderer.hpp"

#include "meta/logger.hpp"
#include "meta_qt/designs/stock/stock.hpp"
#include "meta_qt/ui/design_registry.hpp"

namespace meta::qt
{

MetaWidget *render(AbstractAttribute *p_attr, QWidget *parent)
{
  if (!p_attr)
  {
    Logger::log()->error("incoming p_attr is nullptr");
    return nullptr;
  }

  stock::register_design();
  return DesignRegistry::instance().render(p_attr,
                                           stock::kDesignName,
                                           RowContext{},
                                           parent);
}

} // namespace meta::qt
