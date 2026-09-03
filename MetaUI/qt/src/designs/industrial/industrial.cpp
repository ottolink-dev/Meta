/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/industrial.hpp"

#include "meta_qt/designs/industrial/check_row.hpp"
#include "meta_qt/designs/industrial/combo.hpp"
#include "meta_qt/designs/industrial/int_slider.hpp"
#include "meta_qt/designs/industrial/param_slider.hpp"
#include "meta_qt/designs/industrial/section.hpp"
#include "meta_qt/designs/stock/stock.hpp"
#include "meta_qt/ui/design_registry.hpp"
#include "meta_qt/ui/theme.hpp"

namespace meta::qt::industrial
{

void register_design()
{
  static bool registered = false;
  if (registered) return;
  registered = true;

  DesignRegistry &registry = DesignRegistry::instance();

  // --- Theme & section chrome
  registry.set_theme(kDesignName, ThemeRegistry::kPaletteTheme);
  registry.register_section_factory(
      kDesignName,
      [](const QString &title)
      {
        const Theme &theme = ThemeRegistry::instance().get(
            ThemeRegistry::kPaletteTheme);
        return new Section(title, theme);
      });

  // --- float: 58% of the rows in a Hesiod node panel
  registry.register_control<float, ParamSlider>(kDesignName, "SliderFloat");

  // --- bool: 14%. Both presets map to the same control for now; BinaryButtons
  // wants its own A/B chip pair, which is a separate design entry rather than a
  // branch inside CheckRow.
  registry.register_control<bool, CheckRow>(kDesignName, "Toggle");
  registry.register_control<bool, CheckRow>(kDesignName, "Checkbox");

  // --- int: 13%, counting Seed. Shares its chrome with ParamSlider via
  // slider_chrome so the two rows cannot drift apart.
  registry.register_control<int, IntSlider>(kDesignName, "SliderInt");

  // --- dropdowns. Both use a popup of our own rather than QComboBox: the stock
  // popup composites a separately styled frame and item view, which is why it
  // shows one surface mid-open and another once settled.
  registry.register_control<int, EnumCombo>(kDesignName, "EnumComboBox");
  registry.register_control<std::string, StringCombo>(kDesignName, "ComboBox");
  registry.register_control<std::string, StringCombo>(kDesignName,
                                                      "ButtonGrid");

  // Anything not covered above resolves through stock, so a design still under
  // construction yields a complete panel rather than a handful of rows. Drop
  // this line and the unported widget types simply render nothing.
  stock::register_design();
  registry.set_fallback(kDesignName, stock::kDesignName);
}

} // namespace meta::qt::industrial
