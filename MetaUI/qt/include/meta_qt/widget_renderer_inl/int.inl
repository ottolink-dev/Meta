/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <cmath>
#include <format>

#include <QComboBox>
#include <QLabel>
#include <QWidget>

#include "meta/type/type_name.hpp"
#include "meta_common.hpp"

#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widgets/slider_int.hpp"

namespace meta::qt
{

template <> struct WidgetRenderer<int>
{
  static MetaWidget *render(Attribute<int> &attr, QWidget *parent)
  {
    std::string       widget_type = meta::common::widget_type(attr);
    const std::string label_txt = meta::common::label(attr);
    const std::string format = meta::common::format(attr);
    const int         min = meta::common::min(attr);
    const int         max = meta::common::max(attr);
    const int         step = meta::common::step(attr);
    const auto        items = meta::common::enum_items<int>(attr);
    const bool        plus_minus = meta::common::try_get<bool>(attr,
                                                        "ui.plus_minus",
                                                        false);

    int &value = attr.value();

    MetaWidget *widget = make_meta_widget_vbox(parent);
    auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

    if (!label_txt.empty() && widget_type != "SliderInt")
    {
      QLabel *label = new QLabel(label_txt.c_str(), widget);
      layout->addWidget(label);
    }

    if (widget_type.empty()) widget_type = "Input";

    if (widget_type == "None") // --- None
    {
      return nullptr;
    }
    else if (widget_type == "Input")
    {
      // --- INPUT

      auto *spinbox = new QDoubleSpinBox(widget);

      spinbox->setMinimum(min);
      spinbox->setMaximum(max);
      spinbox->setSingleStep(step);
      spinbox->setDecimals(0);

      // A deserialized value can sit outside the current constraints (e.g.
      // constraints tightened in a later version); make the model agree with
      // the clamped value that is displayed instead of keeping the stale one.
      const int clamped = std::clamp(value, min, max);
      spinbox->setValue(clamped);
      if (clamped != value) attr.set_from_any(clamped);

      layout->addWidget(spinbox);

      widget->set_sync_from_model(
          [spinbox, &value]()
          {
            const QSignalBlocker blocker(spinbox);
            spinbox->setValue(value);
          });

      QObject::connect(
          spinbox,
          &QDoubleSpinBox::valueChanged,
          spinbox,
          [&attr, widget, min, max](double v)
          {
            // Round explicitly: an implicit double -> int conversion
            // truncates toward zero.
            const int iv = static_cast<int>(std::lround(v));
            attr.set_from_any(std::clamp(iv, min, max));
            Q_EMIT widget->edit_started();
            Q_EMIT widget->value_changed();
            Q_EMIT widget->edit_ended();
          });
    }
    else if (widget_type == "EnumComboBox")
    {
      // --- ENUM COMBO BOX

      auto *combo = new QComboBox(widget);
      layout->addWidget(combo);

      int current_index = 0;
      int index = 0;

      for (const auto &[val, name] : items)
      {
        combo->addItem(QString::fromStdString(name), QVariant::fromValue(val));

        if (val == value) current_index = index;

        ++index;
      }

      combo->setCurrentIndex(current_index);

      widget->set_sync_from_model(
          [combo, &value]()
          {
            const QSignalBlocker blocker(combo);

            for (int i = 0; i < combo->count(); ++i)
            {
              if (combo->itemData(i).toInt() == value)
              {
                combo->setCurrentIndex(i);
                break;
              }
            }
          });

      QObject::connect(combo,
                       QOverload<int>::of(&QComboBox::currentIndexChanged),
                       widget,
                       [&attr, widget, combo](int)
                       {
                         attr.set_from_any(combo->currentData().toInt());
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       });
    }
    else if (widget_type == "Slider" || widget_type == "ScrollBar" ||
             widget_type == "Dial")
    {
      if (!attr.metadata().contains_all_keys(
              {meta::keys::constraints::min, meta::keys::constraints::max}))
      {
        layout->addWidget(make_error_widget(&attr, "missing metadata", widget));
        return widget;
      }

      attr.set_from_any(std::clamp(value, min, max));

      QAbstractSlider *control = nullptr;

      if (widget_type == "Slider")
      {
        auto *slider = new QSlider(Qt::Horizontal, widget);
        slider->setRange(min, max);
        slider->setValue(value);
        control = slider;
      }
      else if (widget_type == "ScrollBar")
      {
        auto *scrollbar = new QScrollBar(Qt::Horizontal, widget);
        scrollbar->setRange(min, max);
        scrollbar->setValue(value);
        control = scrollbar;
      }
      else if (widget_type == "Dial")
      {
        auto *dial = new QDial(widget);
        dial->setRange(min, max);
        dial->setValue(value);
        control = dial;
      }

      widget->set_sync_from_model(
          [control, &value]()
          {
            const QSignalBlocker blocker(control);
            control->setValue(value);
          });

      QObject::connect(control,
                       &QAbstractSlider::sliderPressed,
                       widget,
                       [widget]() { Q_EMIT widget->edit_started(); });

      QObject::connect(control,
                       &QAbstractSlider::valueChanged,
                       widget,
                       [&attr, widget, min, max](int v)
                       {
                         attr.set_from_any(std::clamp(v, min, max));
                         Q_EMIT widget->value_changed();
                       });

      QObject::connect(control,
                       &QAbstractSlider::sliderReleased,
                       widget,
                       [widget]() { Q_EMIT widget->edit_ended(); });

      layout->addWidget(control);
    }
    else if (widget_type == "SliderInt") // SliderInt
    {
      auto *slider =
          new SliderInt(label_txt, value, min, max, plus_minus, format, widget);
      slider->set_value(value);
      layout->addWidget(slider);

      widget->set_sync_from_model(
          [slider, &value]()
          {
            const QSignalBlocker blocker(slider);
            slider->set_value(value);
          });

      QObject::connect(slider,
                       &SliderInt::value_changed,
                       widget,
                       [&attr, slider, widget]()
                       {
                         attr.set_from_any(slider->get_value());
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                       });

      QObject::connect(slider,
                       &SliderInt::edit_ended,
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
        [widget](int) { widget->sync_widget_from_model(); });

    return widget;
  }
};

} // namespace meta::qt
