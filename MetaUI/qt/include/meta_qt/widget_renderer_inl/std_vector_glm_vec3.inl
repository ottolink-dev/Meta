/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <glm/glm.hpp>

#include "meta/core/data_provider.hpp"
#include "meta_qt/designs/stock/stock_renderer.hpp"
#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widgets/points_canvas.hpp"

namespace meta::qt::stock
{

template <> struct StockRenderer<std::vector<glm::vec3>>
{
  static MetaWidget *render(Attribute<std::vector<glm::vec3>> &attr,
                            QWidget                           *parent)
  {
    std::string       widget_type = meta::common::widget_type(attr);
    const std::string label_txt = meta::common::label(attr);

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
    const float z_step = meta::common::try_get<float>(attr, "ui.z_step", 0.05f);
    const bool  closed = meta::common::try_get<bool>(attr,
                                                    meta::keys::ui::closed,
                                                    false);

    std::vector<glm::vec3> &value = attr.value();

    MetaWidget *widget = make_meta_widget_vbox(parent);
    auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

    if (widget_type.empty()) widget_type = "PointsEditor";

    const bool is_points = (widget_type == "PointsEditor");
    const bool is_path = (widget_type == "PathEditor");

    if (widget_type == "None") // --- None
    {
      return nullptr;
    }
    else if (is_points || is_path) // --- Point and Path editor
    {
      if (!label_txt.empty())
        layout->addWidget(
            new QLabel(QString::fromStdString(label_txt), widget));

      auto *canvas = new PointsCanvas(value,
                                      min_x,
                                      max_x,
                                      min_y,
                                      max_y,
                                      z_step,
                                      is_path ? PointsCanvas::Mode::Path
                                              : PointsCanvas::Mode::Points,
                                      closed,
                                      widget);
      layout->addWidget(canvas);

      meta::DataProvider points_provider; // empty if none
      if (const auto *mp = attr.metadata().find(meta::keys::ui::data_provider))
        if (const auto
                *dp = mp->try_cast<meta::Attribute<meta::DataProvider>>())
          points_provider = dp->value();

      if (points_provider)
      {
        try
        {
          auto data = points_provider();
          if (auto img = data.get<ImageData>())
            if (img->width > 0 && img->height > 0 && !img->pixels.empty())
              canvas->set_background_image(img->pixels,
                                           img->width,
                                           img->height,
                                           img->channels);
        }
        catch (...)
        {
          // a faulty host provider must not crash the panel
        }
      }

      // --- Toolbar

      auto *toolbar = new QHBoxLayout();

      // Clear
      auto *clear_btn = new QPushButton(QObject::tr("Clear"), widget);
      clear_btn->setFixedHeight(22);
      clear_btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      toolbar->addWidget(clear_btn);

      auto *rand_btn = new QPushButton(QObject::tr("Randomize"), widget);
      rand_btn->setFixedHeight(22);
      rand_btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      toolbar->addWidget(rand_btn);

      // From CSV
      auto *csv_btn = new QPushButton(QObject::tr("From CSV…"), widget);
      csv_btn->setFixedHeight(22);
      csv_btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      toolbar->addWidget(csv_btn);

      layout->addLayout(toolbar);

      // --- Connections

      widget->set_sync_from_model(
          [&value, canvas, widget, points_provider]()
          {
            QSignalBlocker blocker(canvas);
            canvas->set_points(value);

            if (points_provider && !widget->is_editing())
            {
              try
              {
                auto data = points_provider();
                if (auto img = data.get<ImageData>())
                  if (img->width > 0 && img->height > 0 && !img->pixels.empty())
                    canvas->set_background_image(img->pixels,
                                                 img->width,
                                                 img->height,
                                                 img->channels);
              }
              catch (...)
              {
              }
            }
          });

      // Live edits (add / move / z scroll) → edit_started + value_changed
      QObject::connect(canvas,
                       &PointsCanvas::points_changed,
                       widget,
                       [&attr, widget]()
                       {
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();

                         attr.value_changed.notify(attr.value());
                       });

      // Committed edits (drag release / delete / clear / randomize / csv)
      QObject::connect(canvas,
                       &PointsCanvas::drag_ended,
                       widget,
                       [&attr, widget]()
                       {
                         Q_EMIT widget->edit_ended();

                         attr.value_changed.notify(attr.value());
                       });

      // Clear
      QObject::connect(clear_btn,
                       &QPushButton::clicked,
                       canvas,
                       &PointsCanvas::clear_all);

      // Randomize
      QObject::connect(rand_btn,
                       &QPushButton::clicked,
                       widget,
                       [&attr, &value, canvas]()
                       {
                         canvas->randomize(value.size());
                         attr.value_changed.notify(attr.value());
                       });

      // From CSV
      QObject::connect(csv_btn,
                       &QPushButton::clicked,
                       widget,
                       [&attr, canvas, widget]()
                       {
                         const QString path = QFileDialog::getOpenFileName(
                             widget,
                             QObject::tr("Load points from CSV"),
                             QDir::homePath(),
                             QObject::tr("CSV files (*.csv);;All Files (*)"),
                             nullptr,
                             QFileDialog::DontUseNativeDialog);
                         if (!path.isEmpty())
                         {
                           canvas->load_csv(path);
                           attr.value_changed.notify(attr.value());
                         }
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
        [widget](std::vector<glm::vec3>) { widget->sync_widget_from_model(); });

    return widget;
  }
};

} // namespace meta::qt::stock
