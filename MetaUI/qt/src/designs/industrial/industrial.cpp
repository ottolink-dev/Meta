/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/industrial.hpp"

#include "meta_qt/designs/industrial/check_row.hpp"
#include "meta_qt/designs/industrial/param_slider.hpp"
#include "meta_qt/designs/stock/stock.hpp"
#include "meta_qt/ui/design_registry.hpp"

namespace meta::qt::industrial
{

void register_design()
{
  static bool registered = false;
  if (registered) return;
  registered = true;

  DesignRegistry &registry = DesignRegistry::instance();

  // --- float: 58% of the rows in a Hesiod node panel
  registry.register_control<float, ParamSlider>(kDesignName, "SliderFloat");

  // --- bool: 14%. Both presets map to the same control for now; BinaryButtons
  // wants its own A/B chip pair, which is a separate design entry rather than a
  // branch inside CheckRow.
  registry.register_control<bool, CheckRow>(kDesignName, "Toggle");
  registry.register_control<bool, CheckRow>(kDesignName, "Checkbox");

  // Anything not covered above resolves through stock, so a design still under
  // construction yields a complete panel rather than a handful of rows. Drop
  // this line and the unported widget types simply render nothing.
  stock::register_design();
  registry.set_fallback(kDesignName, stock::kDesignName);
}

} // namespace meta::qt::industrial
