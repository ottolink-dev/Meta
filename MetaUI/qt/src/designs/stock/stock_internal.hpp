/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include "meta_qt/ui/design_registry.hpp"

namespace meta::qt::stock
{

void register_stock_bool(DesignRegistry &registry);
void register_stock_numeric(DesignRegistry &registry);
void register_stock_string(DesignRegistry &registry);
void register_stock_filesystem(DesignRegistry &registry);

#ifdef META_ENABLE_GLM_TYPES
void register_stock_glm(DesignRegistry &registry);
#endif

void register_stock_misc(DesignRegistry &registry);

} // namespace meta::qt::stock
