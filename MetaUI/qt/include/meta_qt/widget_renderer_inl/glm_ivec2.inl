/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include "meta_qt/designs/stock/stock_renderer.hpp"
#include "meta_qt/widgets/power_of_two_spin_box.hpp"

namespace meta::qt::stock
{

inline int ceil_power_of_two(int v)
{
  if (v <= 1) return 1;

  int p = 1;

  while (p < v)
    p <<= 1;

  return p;
}

template <> struct StockRenderer<glm::ivec2>
{
  static MetaWidget *render(Attribute<glm::ivec2> &attr, QWidget *parent)
  {
    std::string       widget_type = meta::common::widget_type(attr);
    const std::string label_txt = meta::common::label(attr);
    const int         min = meta::common::min<int>(attr);
    const int         max = meta::common::max<int>(attr);
    const int         step = meta::common::step<int>(attr);
    const bool        power_of_two = meta::common::power_of_two<bool>(attr);
    const float       aspect_ratio = meta::common::aspect_ratio(attr);
    const bool        keep_aspect = (aspect_ratio != 0.f);

    glm::ivec2 &value = attr.value();

    MetaWidget *widget = make_meta_widget_vbox(parent);
    auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

    if (widget_type.empty()) widget_type = "Input";

    if (widget_type == "None") // --- None
    {
      return nullptr;
    }
    else if (widget_type == "Input") // --- Input
    {
      if (!label_txt.empty())
      {
        layout->addWidget(
            new QLabel(QString::fromStdString(label_txt), widget));
      }

      auto *row = new QHBoxLayout();

      QSpinBox *spinbox_x = power_of_two ? new PowerOfTwoSpinBox(widget)
                                         : new QSpinBox(widget);

      QSpinBox *spinbox_y = power_of_two ? new PowerOfTwoSpinBox(widget)
                                         : new QSpinBox(widget);

      for (auto *sp : {spinbox_x, spinbox_y})
      {
        sp->setRange(min, max);
        sp->setSingleStep(step);
      }

      spinbox_x->setValue(std::clamp(value.x, min, max));
      spinbox_y->setValue(std::clamp(value.y, min, max));

      if (keep_aspect)
      {
        spinbox_y->setEnabled(false);
        spinbox_y->setRange(int(min / aspect_ratio), int(max / aspect_ratio));
        spinbox_y->setToolTip(
            QString(QObject::tr("Aspect ratio x/y = %1")).arg(aspect_ratio));

        // synchronize the initial value
        int y = int(std::lround(double(spinbox_x->value()) / aspect_ratio));
        value.x = spinbox_x->value();
        value.y = y;

        spinbox_y->setValue(y);
      }

      row->addWidget(spinbox_x);
      row->addWidget(spinbox_y);

      layout->addLayout(row);

      widget->set_sync_from_model(
          [spinbox_x,
           spinbox_y,
           &value,
           min,
           max,
           power_of_two,
           keep_aspect,
           aspect_ratio]()
          {
            int x = std::clamp(value.x, min, max);
            int y = std::clamp(value.y, min, max);

            if (power_of_two)
            {
              x = ceil_power_of_two(x);
              y = ceil_power_of_two(y);
            }

            if (keep_aspect) y = int(std::lround(double(x) / aspect_ratio));

            {
              QSignalBlocker blocker(spinbox_x);
              spinbox_x->setValue(x);
            }

            {
              QSignalBlocker blocker(spinbox_y);
              spinbox_y->setValue(y);
            }
          });

      QObject::connect(spinbox_x,
                       qOverload<int>(&QSpinBox::valueChanged),
                       widget,
                       [&value,
                        &attr,
                        widget,
                        spinbox_x,
                        spinbox_y,
                        min,
                        max,
                        power_of_two,
                        keep_aspect,
                        aspect_ratio](int v)
                       {
                         int x = std::clamp(v, min, max);

                         if (power_of_two) x = ceil_power_of_two(x);

                         {
                           QSignalBlocker blocker(spinbox_x);
                           spinbox_x->setValue(x);
                         }

                         glm::ivec2 new_value = {x, 0};

                         if (keep_aspect)
                         {
                           int y = int(std::lround(double(x) / aspect_ratio));
                           new_value.y = y;

                           QSignalBlocker blocker(spinbox_y);
                           spinbox_y->setValue(y);
                         }

                         attr.set_from_any(new_value);

                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       });

      if (!keep_aspect)
      {
        QObject::connect(
            spinbox_y,
            qOverload<int>(&QSpinBox::valueChanged),
            widget,
            [&value, &attr, widget, spinbox_y, min, max, power_of_two](int v)
            {
              int y = std::clamp(v, min, max);

              if (power_of_two) y = ceil_power_of_two(y);

              {
                QSignalBlocker blocker(spinbox_y);
                spinbox_y->setValue(y);
              }

              attr.set_from_any(glm::ivec2{value.x, y});

              Q_EMIT widget->edit_started();
              Q_EMIT widget->value_changed();
              Q_EMIT widget->edit_ended();
            });
      }
    }
    else // --- ERROR
    {
      layout->addWidget(
          make_error_widget(&attr, "unsupported widget type", widget));
    }

    // connection: attribute changed ==> widget update (dies with the
    // widget destruction)
    widget->connection_ = attr.value_changed.subscribe(
        [widget](glm::ivec2) { widget->sync_widget_from_model(); });

    return widget;
  }
};

} // namespace meta::qt::stock
