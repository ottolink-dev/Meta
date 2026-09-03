/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once

#include <QLabel>
#include <QVBoxLayout>

#include "meta_common.hpp"
#include "meta_qt/designs/stock/stock_renderer.hpp"
#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widgets/gradient_picker.hpp"

#include "meta/ext/color_gradient/color_gradient.hpp"
#include "meta/metadata/keys.hpp"

namespace meta::qt::stock
{

// ---------------------------------------------------------------------------
// StockRenderer<ColorGradient>
//
// widget_type: "GradientEditor" (default)
//
// Stops come from the attribute value; presets come from the ui.presets
// metadata entry (GradientPresets), installed by the host at setup time.
// ---------------------------------------------------------------------------

template <> struct StockRenderer<meta::ColorGradient>
{
  static MetaWidget *render(Attribute<meta::ColorGradient> &attr,
                            QWidget                        *parent)
  {
    std::string       widget_type = meta::common::widget_type(attr);
    const std::string label_txt = meta::common::label(attr);

    ColorGradient &cga = attr.value();

    MetaWidget *widget = make_meta_widget_vbox(parent);
    auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

    if (widget_type.empty()) widget_type = "GradientEditor";

    if (widget_type == "None") // --- None
    {
      return nullptr;
    }
    else if (widget_type == "GradientEditor") // --- GradientEditor
    {
      if (!label_txt.empty())
        layout->addWidget(
            new QLabel(QString::fromStdString(label_txt), widget));

      const auto *p_presets = attr.metadata().try_value<meta::GradientPresets>(
          meta::keys::ui::presets);

      auto *picker = new GradientPicker(cga.value(),
                                        p_presets ? p_presets->presets
                                                  : std::vector<meta::Preset>{},
                                        widget);
      layout->addWidget(picker);

      widget->set_sync_from_model(
          [picker]()
          {
            picker->update_bar();
            picker->update();
          });

      // Live edits
      QObject::connect(picker,
                       &GradientPicker::value_changed,
                       widget,
                       [&attr, widget]()
                       {
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();

                         attr.value_changed.notify(attr.value());
                       });

      // Committed
      QObject::connect(picker,
                       &GradientPicker::edit_ended,
                       widget,
                       [&attr, widget]()
                       {
                         Q_EMIT widget->edit_ended();
                         attr.value_changed.notify(attr.value());
                       });
    }
    else // --- ERROR
    {
      layout->addWidget(
          make_error_widget(&attr, "unsupported widget type", widget));
    }

    // connection: attribute changed ==> widget update (dies with the
    // widget destruction)
    widget->connection_ = attr.value_changed.subscribe(
        [widget](meta::ColorGradient) { widget->sync_widget_from_model(); });

    return widget;
  }
};

} // namespace meta::qt::stock
