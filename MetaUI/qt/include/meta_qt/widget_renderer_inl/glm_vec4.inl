/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <QColorDialog>
#include <QFontDatabase>

#include "meta_qt/designs/stock/stock_renderer.hpp"

namespace meta::qt::stock
{

template <> struct StockRenderer<glm::vec4>
{
  static MetaWidget *render(Attribute<glm::vec4> &attr, QWidget *parent)
  {
    std::string       widget_type = meta::common::widget_type(attr);
    const std::string label_txt = meta::common::label(attr);
    const std::string format = meta::common::format(attr);
    const float       min = meta::common::min<float>(attr);
    const float       max = meta::common::max<float>(attr);
    const float       step = meta::common::step<float>(attr);

    const int decimals = meta::common::try_get_format_decimals(format);

    glm::vec4 &value = attr.value();

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

      auto *spinbox_x = new QDoubleSpinBox(widget);
      auto *spinbox_y = new QDoubleSpinBox(widget);
      auto *spinbox_z = new QDoubleSpinBox(widget);
      auto *spinbox_w = new QDoubleSpinBox(widget);

      for (auto *sp : {spinbox_x, spinbox_y, spinbox_z, spinbox_w})
      {
        sp->setRange(min, max);
        sp->setSingleStep(step);
        sp->setDecimals(decimals);
      }

      spinbox_x->setValue(std::clamp(value.x, min, max));
      spinbox_y->setValue(std::clamp(value.y, min, max));
      spinbox_z->setValue(std::clamp(value.z, min, max));
      spinbox_w->setValue(std::clamp(value.w, min, max));

      row->addWidget(spinbox_x);
      row->addWidget(spinbox_y);
      row->addWidget(spinbox_z);
      row->addWidget(spinbox_w);

      layout->addLayout(row);

      widget->set_sync_from_model(
          [&value, spinbox_x, spinbox_y, spinbox_z, spinbox_w]()
          {
            {
              QSignalBlocker bx(spinbox_x);
              spinbox_x->setValue(std::clamp(value.x, 0.f, 1.f));
            }

            {
              QSignalBlocker by(spinbox_y);
              spinbox_y->setValue(std::clamp(value.y, 0.f, 1.f));
            }

            {
              QSignalBlocker bz(spinbox_z);
              spinbox_z->setValue(std::clamp(value.z, 0.f, 1.f));
            }

            {
              QSignalBlocker bz(spinbox_w);
              spinbox_w->setValue(std::clamp(value.w, 0.f, 1.f));
            }
          });

      QObject::connect(spinbox_x,
                       qOverload<double>(&QDoubleSpinBox::valueChanged),
                       widget,
                       [&attr, &value, widget, spinbox_x, min, max](double v)
                       {
                         float x = std::clamp(static_cast<float>(v), min, max);

                         {
                           QSignalBlocker blocker(spinbox_x);
                           spinbox_x->setValue(x);
                         }

                         value.x = x;

                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });

      QObject::connect(spinbox_y,
                       qOverload<double>(&QDoubleSpinBox::valueChanged),
                       widget,
                       [&attr, &value, widget, spinbox_y, min, max](double v)
                       {
                         float y = std::clamp(static_cast<float>(v), min, max);

                         {
                           QSignalBlocker blocker(spinbox_y);
                           spinbox_y->setValue(y);
                         }

                         value.y = y;

                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });

      QObject::connect(spinbox_z,
                       qOverload<double>(&QDoubleSpinBox::valueChanged),
                       widget,
                       [&attr, &value, widget, spinbox_z, min, max](double v)
                       {
                         float z = std::clamp(static_cast<float>(v), min, max);

                         {
                           QSignalBlocker blocker(spinbox_z);
                           spinbox_z->setValue(z);
                         }

                         value.z = z;

                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });

      QObject::connect(spinbox_w,
                       qOverload<double>(&QDoubleSpinBox::valueChanged),
                       widget,
                       [&attr, &value, widget, spinbox_w, min, max](double v)
                       {
                         float w = std::clamp(static_cast<float>(v), min, max);

                         {
                           QSignalBlocker blocker(spinbox_w);
                           spinbox_w->setValue(w);
                         }

                         value.w = w;

                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });
    }
    else if (widget_type == "ColorPicker") // --- ColorPicker
    {
      if (!label_txt.empty())
      {
        layout->addWidget(
            new QLabel(QString::fromStdString(label_txt), widget));
      }

      auto *row = new QHBoxLayout();

      auto *color_button = new QPushButton(widget);
      color_button->setFixedSize(48, 24);
      color_button->setFlat(true);

      // Checkerboard background visible through transparent swatches.
      // Painted as a stylesheet with a QPixmap-backed pattern would require
      // subclassing; instead we use a simple two-tone gradient approximation
      // that is Good Enough for a swatch at this size.
      auto update_button_color = [color_button](const glm::vec4 &v)
      {
        const int r = static_cast<int>(std::clamp(v.x, 0.f, 1.f) * 255.f);
        const int g = static_cast<int>(std::clamp(v.y, 0.f, 1.f) * 255.f);
        const int b = static_cast<int>(std::clamp(v.z, 0.f, 1.f) * 255.f);
        const int a = static_cast<int>(std::clamp(v.w, 0.f, 1.f) * 255.f);
        color_button->setStyleSheet(
            QString(
                "background-color: rgba(%1,%2,%3,%4); border: 1px solid gray;")
                .arg(r)
                .arg(g)
                .arg(b)
                .arg(a));
      };

      update_button_color(value);
      row->addWidget(color_button);

      // Hex label including alpha channel (#RRGGBBAA)
      auto *hex_label = new QLabel(widget);
      hex_label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

      auto update_hex_label = [hex_label](const glm::vec4 &v)
      {
        const int r = static_cast<int>(std::clamp(v.x, 0.f, 1.f) * 255.f);
        const int g = static_cast<int>(std::clamp(v.y, 0.f, 1.f) * 255.f);
        const int b = static_cast<int>(std::clamp(v.z, 0.f, 1.f) * 255.f);
        const int a = static_cast<int>(std::clamp(v.w, 0.f, 1.f) * 255.f);
        hex_label->setText(QString("#%1%2%3%4")
                               .arg(r, 2, 16, QChar('0'))
                               .arg(g, 2, 16, QChar('0'))
                               .arg(b, 2, 16, QChar('0'))
                               .arg(a, 2, 16, QChar('0'))
                               .toUpper());
      };

      update_hex_label(value);
      row->addWidget(hex_label);
      row->addStretch();

      layout->addLayout(row);

      widget->set_sync_from_model(
          [&value, update_button_color, update_hex_label]()
          {
            update_button_color(value);
            update_hex_label(value);
          });

      QObject::connect(
          color_button,
          &QPushButton::clicked,
          widget,
          [&attr, &value, widget, update_button_color, update_hex_label]()
          {
            const int    ri = static_cast<int>(std::clamp(value.x, 0.f, 1.f) *
                                            255.f);
            const int    gi = static_cast<int>(std::clamp(value.y, 0.f, 1.f) *
                                            255.f);
            const int    bi = static_cast<int>(std::clamp(value.z, 0.f, 1.f) *
                                            255.f);
            const int    ai = static_cast<int>(std::clamp(value.w, 0.f, 1.f) *
                                            255.f);
            const QColor initial(ri, gi, bi, ai);

            const QColor picked = QColorDialog::getColor(
                initial,
                widget,
                QString(),
                QColorDialog::ShowAlphaChannel |
                    QColorDialog::DontUseNativeDialog);

            if (!picked.isValid()) return;

            value.x = static_cast<float>(picked.redF());
            value.y = static_cast<float>(picked.greenF());
            value.z = static_cast<float>(picked.blueF());
            value.w = static_cast<float>(picked.alphaF());

            update_button_color(value);
            update_hex_label(value);

            Q_EMIT widget->edit_started();
            Q_EMIT widget->value_changed();
            Q_EMIT widget->edit_ended();

            // not using 'set_from_any' method, force emit
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
        [widget](glm::vec4) { widget->sync_widget_from_model(); });

    return widget;
  }
};

} // namespace meta::qt::stock
