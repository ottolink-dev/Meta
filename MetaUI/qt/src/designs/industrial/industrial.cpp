/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/industrial.hpp"

#include "meta_qt/designs/industrial/check_row.hpp"
#include "meta_qt/designs/industrial/editor_style.hpp"
#include "meta_qt/widgets/range_bar.hpp"
#ifdef META_ENABLE_ARRAY_TYPES
#include "meta/ext/array/array.hpp"
#include "meta_qt/widgets/array_canvas.hpp"
#include <QVBoxLayout>
#endif
#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
#include "meta/ext/color_gradient/color_gradient.hpp"
#endif
#include "meta_qt/designs/industrial/combo.hpp"
#include "meta_qt/designs/industrial/int_slider.hpp"
#include "meta_qt/designs/industrial/linked_sliders.hpp"
#include "meta_qt/designs/industrial/param_slider.hpp"
#include "meta_qt/designs/industrial/section.hpp"
#include "meta_qt/designs/industrial/text_row.hpp"
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
        // Read the design's theme rather than naming one. set_theme() exists so
        // a host can pick the colourway, and pinning the palette theme here
        // ignored it: the rows followed the setting while the section cards
        // stayed on the app palette, so the two drew different greys.
        //
        // Resolved per section rather than captured, because the theme can be
        // set after the design registers.
        return new Section(title,
                           DesignRegistry::instance().theme(kDesignName));
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

  // --- std::string text rows: 20 rows. Both flavours are the same control,
  // ReadOnlyText simply never accepts typing.
  registry.register_control<std::string, TextRow>(kDesignName,
                                                  "SingleLineText");
  registry.register_control<std::string, TextRow>(kDesignName, "ReadOnlyText");

#ifdef META_ENABLE_GLM_TYPES
  // --- glm::vec2: 41 rows, all of them Spatial Frequency. Two rails and a
  // link, sharing slider_chrome with the single-value rows above.
  registry.register_control<glm::vec2, LinkedSliders>(kDesignName,
                                                      "LinkedSliders");
#endif

  // Anything not covered above resolves through stock, so a design still under
  // construction yields a complete panel rather than a handful of rows. Drop
  // this line and the unported widget types simply render nothing.
  stock::register_design();
  const RowFactory editor = [](AbstractAttribute &attr,
                               const RowContext  &ctx,
                               QWidget           *parent) -> MetaWidget *
  {
    auto *widget = DesignRegistry::instance().render(&attr,
                                                     stock::kDesignName,
                                                     ctx,
                                                     parent);
    if (!widget) return nullptr;
#ifdef META_ENABLE_ARRAY_TYPES
    if (auto *canvas = widget->findChild<ArrayCanvas *>())
    {
      auto *layout = qobject_cast<QVBoxLayout *>(widget->layout());
      layout->insertWidget(
          0,
          new QLabel(QString::fromStdString(meta::common::label(
                         static_cast<Attribute<meta::Array> &>(attr))),
                     widget));
      auto *hint = new QLabel(
          QObject::tr("Drag to paint · Right-drag to erase · Scroll to resize"),
          widget);
      hint->setWordWrap(true);
      layout->addWidget(hint);
      canvas->setProperty("industrialEditor", true);
    }
#endif
    style_editor(widget,
                 ctx.theme ? *ctx.theme
                           : DesignRegistry::instance().theme(kDesignName));
    for (auto *range : widget->findChildren<RangeBar *>())
    {
      range->set_theme(ctx.theme
                           ? *ctx.theme
                           : DesignRegistry::instance().theme(kDesignName));
      range->setFixedHeight(60);
    }
    return widget;
  };
#ifdef META_ENABLE_GLM_TYPES
  registry.add(kDesignName, typeid(glm::vec2), "RangeBar", editor);
  registry.add(kDesignName,
               typeid(std::vector<glm::vec3>),
               "PathEditor",
               editor);
  registry.add(kDesignName,
               typeid(std::vector<glm::vec3>),
               "PointsEditor",
               editor);
#endif
#ifdef META_ENABLE_ARRAY_TYPES
  registry.add(kDesignName, typeid(meta::Array), kAnyWidgetType, editor);
#endif
#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
  registry.add(kDesignName,
               typeid(meta::ColorGradient),
               kAnyWidgetType,
               editor);
#endif
  registry.set_fallback(kDesignName, stock::kDesignName);
}

} // namespace meta::qt::industrial
