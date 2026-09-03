/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/stock/stock.hpp"

#include "stock_internal.hpp"

#include "meta_qt/ui/design_registry.hpp"
#include "meta_qt/widgets/collapsible_section.hpp"

namespace meta::qt::stock
{

void register_design()
{
  static bool registered = false;
  if (registered) return;
  registered = true;

  DesignRegistry &registry = DesignRegistry::instance();

  // --- Section factory
  registry.register_section_factory(kDesignName,
                                    [](const QString &title)
                                    { return new CollapsibleSection(title); });

  // --- Category widget registrations
  register_stock_bool(registry);
  register_stock_numeric(registry);
  register_stock_string(registry);
  register_stock_filesystem(registry);

#ifdef META_ENABLE_GLM_TYPES
  register_stock_glm(registry);
#endif

  register_stock_misc(registry);
}

} // namespace meta::qt::stock
