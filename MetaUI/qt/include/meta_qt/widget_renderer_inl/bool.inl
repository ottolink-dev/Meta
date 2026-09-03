/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <QCheckBox>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QWidget>

#include "meta/type/type_name.hpp"
#include "meta_common.hpp"

#include "meta_qt/designs/stock/stock_renderer.hpp"
#include "meta_qt/meta_widget.hpp"

namespace meta::qt::stock
{

template <> struct StockRenderer<bool>
{
  static MetaWidget *render(Attribute<bool> &attr, QWidget *parent)
  {
    std::string       widget_type = meta::common::widget_type(attr);
    const std::string label_txt = meta::common::label(attr);
    bool             &value = attr.value();

    MetaWidget *widget = make_meta_widget_grid(parent);
    auto       *layout = dynamic_cast<QGridLayout *>(widget->layout());

    if (widget_type.empty()) widget_type = "Toggle";

    if (widget_type == "None") // --- None
    {
      return nullptr;
    }
    else if (widget_type == "Toggle") // --- Togglr
    {
      auto *button = new QPushButton(label_txt.c_str(), widget);
      layout->addWidget(button, 0, 0);

      button->setCheckable(true);

      widget->set_sync_from_model([button, &value]()
                                  { button->setChecked(value); });

      widget->sync_widget_from_model();

      QObject::connect(button,
                       &QPushButton::toggled,
                       widget,
                       [widget, &attr](bool checked)
                       {
                         attr.set_from_any(checked);
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       });
    }
    else if (widget_type == "BinaryButtons") // --- BinaryButtons
    {
      int row = 0;

      if (!label_txt.empty())
      {
        QLabel *label = new QLabel(label_txt.c_str(), widget);
        layout->addWidget(label, row, 0, 1, 2);
        row++;
      }

      const std::string label_true = meta::common::try_get<std::string>(
          attr,
          meta::keys::ui::label_true,
          "True");
      const std::string label_false = meta::common::try_get<std::string>(
          attr,
          meta::keys::ui::label_false,
          "False");

      auto *button_true = new QPushButton(QObject::tr(label_true.c_str()),
                                          widget);
      auto *button_false = new QPushButton(QObject::tr(label_false.c_str()),
                                           widget);

      layout->addWidget(button_true, row, 0);
      layout->addWidget(button_false, row, 1);

      // make the buttons checkable
      button_true->setCheckable(true);
      button_false->setCheckable(true);

      // set the initial state of the buttons based on the attribute value
      widget->set_sync_from_model(
          [button_true, button_false, &value]()
          {
            button_true->setChecked(value);
            button_false->setChecked(!value);
          });

      widget->sync_widget_from_model();

      // connect the buttons' clicked signals to update the state
      QObject::connect(button_true,
                       &QPushButton::clicked,
                       widget,
                       [widget, button_true, button_false, &attr]()
                       {
                         if (button_true->isChecked())
                         {
                           button_false->setChecked(false);
                           attr.set_from_any(true);
                           Q_EMIT widget->edit_started();
                           Q_EMIT widget->value_changed();
                           Q_EMIT widget->edit_ended();
                         }
                         else
                         {
                           // ensure at least one button is always checked
                           button_true->setChecked(true);
                         }
                       });

      QObject::connect(button_false,
                       &QPushButton::clicked,
                       widget,
                       [widget, button_true, button_false, &attr]()
                       {
                         if (button_false->isChecked())
                         {
                           button_true->setChecked(false);
                           attr.set_from_any(false);
                           Q_EMIT widget->edit_started();
                           Q_EMIT widget->value_changed();
                           Q_EMIT widget->edit_ended();
                         }
                         else
                         {
                           button_false->setChecked(true);
                         }
                       });
    }
    else if (widget_type == "Checkbox") // --- Checkbox
    {
      QCheckBox *checkbox = new QCheckBox(label_txt.c_str(), widget);
      layout->addWidget(checkbox, 0, 0);

      widget->set_sync_from_model([checkbox, &value]()
                                  { checkbox->setChecked(value); });

      widget->sync_widget_from_model();

      checkbox->connect(checkbox,
                        &QCheckBox::toggled,
                        checkbox,
                        [widget, checkbox, &attr](bool checked)
                        {
                          attr.set_from_any(checked);
                          Q_EMIT widget->edit_started();
                          Q_EMIT widget->value_changed();
                          Q_EMIT widget->edit_ended();
                        });
    }
    else // --- ERROR
    {
      layout->addWidget(
          make_error_widget(&attr, "unsupported widget type", widget),
          0,
          0);
    }

    // connection: attribute changed ==> widget update (dies with the
    // widget destruction)
    widget->connection_ = attr.value_changed.subscribe(
        [widget](bool) { widget->sync_widget_from_model(); });

    return widget;
  }
};

} // namespace meta::qt::stock
