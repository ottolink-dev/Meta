/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <format>

#include <QDial>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QScrollBar>
#include <QSlider>
#include <QWidget>

#include "meta/type/type_name.hpp"
#include "meta_common.hpp"

#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widgets/slider_float.hpp"

namespace meta::qt
{

template <> struct WidgetRenderer<float>
{
  static MetaWidget *render(Attribute<float> &attr, QWidget *parent)
  {
    std::string       widget_type = meta::common::widget_type(attr);
    const std::string label_txt = meta::common::label(attr);
    const std::string format = meta::common::format(attr);
    const float       min = meta::common::min(attr);
    const float       max = meta::common::max(attr);
    const float       step = meta::common::step(attr);
    const bool        plus_minus = meta::common::try_get<bool>(attr,
                                                        "ui.plus_minus",
                                                        false);
    const bool        log_scale = meta::common::try_get<bool>(
        attr,
        meta::keys::ui::log_scale,
        false);

    float &value = attr.value();

    MetaWidget *widget = make_meta_widget_vbox(parent);
    auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

    if (!label_txt.empty() && widget_type != "SliderFloat")
    {
      QLabel *label = new QLabel(label_txt.c_str(), widget);
      layout->addWidget(label);
    }

    if (widget_type.empty()) widget_type = "Input";

    if (widget_type == "None") // --- None
    {
      return nullptr;
    }
    else if (widget_type == "Input") // --- Input
    {
      // --- INPUT

      auto *spinbox = new QDoubleSpinBox(widget);

      spinbox->setMinimum(min);
      spinbox->setMaximum(max);
      spinbox->setSingleStep(step);
      spinbox->setValue(std::clamp(value, min, max));
      spinbox->setDecimals(meta::common::try_get_format_decimals(format));

      layout->addWidget(spinbox);

      widget->set_sync_from_model(
          [spinbox, &value]()
          {
            const QSignalBlocker blocker(spinbox);
            spinbox->setValue(value);
          });

      QObject::connect(spinbox,
                       &QDoubleSpinBox::valueChanged,
                       spinbox,
                       [&attr, widget, min, max](double v)
                       {
                         attr.set_from_any(
                             std::clamp(static_cast<float>(v), min, max));
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       });
    }
    else if (widget_type == "Slider" || widget_type == "ScrollBar" ||
             widget_type == "Dial") // --- Sliders
    {
      if (!attr.metadata().contains_all_keys(
              {meta::keys::constraints::min, meta::keys::constraints::max}))
      {
        layout->addWidget(make_error_widget(&attr, "missing metadata", widget));
        return widget;
      }

      constexpr int range_min = 0;
      constexpr int range_max = 1000;

      auto to_int = [min, max](float v) -> int
      { return static_cast<int>(((v - min) / (max - min)) * range_max); };

      auto from_int = [min, max](int v) -> float
      { return min + (static_cast<float>(v) / range_max) * (max - min); };

      attr.set_from_any(std::clamp(value, min, max));

      QAbstractSlider *control = nullptr;

      if (widget_type == "Slider")
      {
        auto *slider = new QSlider(Qt::Horizontal, widget);
        slider->setRange(range_min, range_max);
        slider->setValue(to_int(value));
        control = slider;
      }
      else if (widget_type == "ScrollBar")
      {
        auto *scrollbar = new QScrollBar(Qt::Horizontal, widget);
        scrollbar->setRange(range_min, range_max);
        scrollbar->setValue(to_int(value));
        control = scrollbar;
      }
      else if (widget_type == "Dial")
      {
        auto *dial = new QDial(widget);
        dial->setRange(range_min, range_max);
        dial->setValue(to_int(value));
        control = dial;
      }

      widget->set_sync_from_model(
          [control, &value, min, max]()
          {
            auto to_int = [min, max](float v) -> int
            { return static_cast<int>(((v - min) / (max - min)) * 1000); };

            const QSignalBlocker blocker(control);
            control->setValue(to_int(value));
          });

      QObject::connect(control,
                       &QAbstractSlider::sliderPressed,
                       widget,
                       [widget]() { Q_EMIT widget->edit_started(); });

      QObject::connect(control,
                       &QAbstractSlider::valueChanged,
                       widget,
                       [&attr, widget, from_int, min, max](int v)
                       {
                         attr.set_from_any(std::clamp(from_int(v), min, max));
                         Q_EMIT widget->value_changed();
                       });

      QObject::connect(control,
                       &QAbstractSlider::sliderReleased,
                       widget,
                       [widget]() { Q_EMIT widget->edit_ended(); });

      layout->addWidget(control);
    }
    else if (widget_type == "SliderFloat") // --- SliderFloat
    {
      auto *slider = new SliderFloat(label_txt,
                                     value,
                                     min,
                                     max,
                                     plus_minus,
                                     format,
                                     log_scale,
                                     widget);
      slider->set_value(value);
      layout->addWidget(slider);

      widget->set_sync_from_model(
          [slider, &value]()
          {
            const QSignalBlocker blocker(slider);
            slider->set_value(value);
          });

      // Live drag / +- buttons
      QObject::connect(slider,
                       &SliderFloat::value_changed,
                       widget,
                       [&attr, slider, widget]()
                       {
                         attr.set_from_any(slider->get_value());
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                       });

      // Committed (drag release, double-click confirm, context menu action)
      QObject::connect(slider,
                       &SliderFloat::edit_ended,
                       widget,
                       [&attr, slider, widget]()
                       {
                         attr.set_from_any(slider->get_value());
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
        [widget](float) { widget->sync_widget_from_model(); });

    return widget;
  }
};

} // namespace meta::qt
