/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#include "meta/logger.hpp"

#include "meta_qt/designs/stock/stock_renderer.hpp"
#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widgets/curve_canvas.hpp"

namespace meta::qt::stock
{

template <> struct StockRenderer<std::vector<float>>
{
  static MetaWidget *render(Attribute<std::vector<float>> &attr,
                            QWidget                       *parent)
  {
    std::vector<float> &value = attr.value();
    const int           default_size = value.size() ? value.size() : 16;

    std::string       widget_type = meta::common::widget_type(attr);
    const std::string label_txt = meta::common::label(attr);

    const int   curve_size = meta::common::try_get<int>(attr,
                                                      "ui.curve_size",
                                                      default_size);
    const float min_x = meta::common::try_get<float>(attr,
                                                     meta::keys::ui::min_x,
                                                     0.f);
    const float max_x = meta::common::try_get<float>(attr,
                                                     meta::keys::ui::max_x,
                                                     1.f);
    const float min_y = meta::common::try_get<float>(attr,
                                                     meta::keys::ui::min_y,
                                                     0.f);
    const float max_y = meta::common::try_get<float>(attr,
                                                     meta::keys::ui::max_y,
                                                     1.f);

    if (widget_type.empty()) widget_type = "CurveEditor";

    MetaWidget *widget = make_meta_widget_vbox(parent);
    auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

    if (!label_txt.empty())
      layout->addWidget(new QLabel(QString::fromStdString(label_txt), widget));

    if (widget_type == "None") // --- None
    {
      return nullptr;
    }
    else if (widget_type == "CurveEditor") // --- CurveEditor
    {
      auto *canvas = new CurveCanvas(value,
                                     curve_size,
                                     min_x,
                                     max_x,
                                     min_y,
                                     max_y,
                                     widget);
      layout->addWidget(canvas);

      // reset button
      auto *btn_row = new QHBoxLayout();
      auto *reset_btn = new QPushButton(QObject::tr("Reset"), widget);
      reset_btn->setFixedHeight(22);
      reset_btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      btn_row->addStretch();
      btn_row->addWidget(reset_btn);
      layout->addLayout(btn_row);

      widget->set_sync_from_model(
          [canvas]()
          {
            const QSignalBlocker blocker(canvas);
            canvas->update();
          });

      // propagate canvas changes to the node graph.
      QObject::connect(canvas,
                       &CurveCanvas::curve_changed,
                       widget,
                       [widget, &attr]()
                       {
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();

                         // not using 'set_from_any' method, emit the
                         // signal manually
                         attr.value_changed.notify(attr.value());
                       });

      QObject::connect(canvas,
                       &CurveCanvas::drag_ended,
                       widget,
                       [widget, &attr]()
                       {
                         Q_EMIT widget->edit_ended();

                         // not using 'set_from_any' method, emit the signal
                         // manually
                         attr.value_changed.notify(attr.value());
                       });

      // reset to identity diagonal.
      QObject::connect(reset_btn,
                       &QPushButton::clicked,
                       widget,
                       [&attr, curve_size, min_y, max_y, canvas, widget]()
                       {
                         std::vector<float> new_value;
                         new_value.reserve(curve_size);

                         for (int i = 0; i < curve_size; ++i)
                         {
                           const float t = float(i) / float(curve_size - 1);
                           new_value.push_back(min_y + t * (max_y - min_y));
                         }

                         attr.set_from_any(new_value);

                         // re-create the canvas state from the new buffer.
                         canvas->reset_to_value();

                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
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
        [widget](std::vector<float>) { widget->sync_widget_from_model(); });

    return widget;
  }
};

} // namespace meta::qt::stock
