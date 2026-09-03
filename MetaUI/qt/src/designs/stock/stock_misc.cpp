/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "stock_internal.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "meta_common.hpp"
#include "meta_qt/designs/stock/stock.hpp"
#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widgets/array_canvas.hpp"
#include "meta_qt/widgets/curve_canvas.hpp"
#include "meta_qt/widgets/gradient_picker.hpp"
#include "meta_qt/widgets/points_canvas.hpp"

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
#include "meta/ext/color_gradient/color_gradient.hpp"
#endif

#ifdef META_ENABLE_ARRAY_TYPES
#include "meta/core/data_provider.hpp"
#include "meta/ext/array/array.hpp"
#endif

namespace meta::qt::stock
{

namespace
{

MetaWidget *render_vec_float(AbstractAttribute &abstract_attr,
                             const RowContext &,
                             QWidget *parent)
{
  auto &attr = static_cast<Attribute<std::vector<float>> &>(abstract_attr);
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

  if (widget_type == "None")
  {
    return nullptr;
  }
  else if (widget_type == "CurveEditor")
  {
    auto *canvas =
        new CurveCanvas(value, curve_size, min_x, max_x, min_y, max_y, widget);
    layout->addWidget(canvas);

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

    QObject::connect(canvas,
                     &CurveCanvas::curve_changed,
                     widget,
                     [widget, &attr]()
                     {
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(canvas,
                     &CurveCanvas::drag_ended,
                     widget,
                     [widget, &attr]()
                     {
                       Q_EMIT widget->edit_ended();
                       attr.value_changed.notify(attr.value());
                     });

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
                       canvas->reset_to_value();

                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
                     });
  }
  else
  {
    layout->addWidget(
        make_error_widget(&attr, "unsupported widget type", widget));
  }

  widget->connection_ = attr.value_changed.subscribe(
      [widget](std::vector<float>) { widget->sync_widget_from_model(); });

  return widget;
}

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
MetaWidget *render_color_gradient(AbstractAttribute &abstract_attr,
                                  const RowContext &,
                                  QWidget *parent)
{
  auto &attr = static_cast<Attribute<meta::ColorGradient> &>(abstract_attr);
  std::string       widget_type = meta::common::widget_type(attr);
  const std::string label_txt = meta::common::label(attr);

  meta::ColorGradient &cga = attr.value();

  MetaWidget *widget = make_meta_widget_vbox(parent);
  auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

  if (widget_type.empty()) widget_type = "GradientEditor";

  if (widget_type == "None")
  {
    return nullptr;
  }
  else if (widget_type == "GradientEditor")
  {
    if (!label_txt.empty())
      layout->addWidget(new QLabel(QString::fromStdString(label_txt), widget));

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

    QObject::connect(picker,
                     &GradientPicker::value_changed,
                     widget,
                     [&attr, widget]()
                     {
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(picker,
                     &GradientPicker::edit_ended,
                     widget,
                     [&attr, widget]()
                     {
                       Q_EMIT widget->edit_ended();
                       attr.value_changed.notify(attr.value());
                     });
  }
  else
  {
    layout->addWidget(
        make_error_widget(&attr, "unsupported widget type", widget));
  }

  widget->connection_ = attr.value_changed.subscribe(
      [widget](meta::ColorGradient) { widget->sync_widget_from_model(); });

  return widget;
}
#endif

#ifdef META_ENABLE_ARRAY_TYPES

inline std::vector<float> resample_bilinear_array(const std::vector<float> &src,
                                                  int src_w,
                                                  int src_h,
                                                  int dst_w,
                                                  int dst_h)
{
  if (src.empty() || src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
    return std::vector<float>(static_cast<size_t>(dst_w * dst_h), 0.f);

  auto at = [&](int x, int y) -> float
  {
    x = std::clamp(x, 0, src_w - 1);
    y = std::clamp(y, 0, src_h - 1);
    return src[static_cast<size_t>(y * src_w + x)];
  };

  std::vector<float> dst(static_cast<size_t>(dst_w * dst_h));
  const float x_scale = static_cast<float>(src_w) / static_cast<float>(dst_w);
  const float y_scale = static_cast<float>(src_h) / static_cast<float>(dst_h);

  for (int j = 0; j < dst_h; ++j)
  {
    for (int i = 0; i < dst_w; ++i)
    {
      const float gx = (static_cast<float>(i) + 0.5f) * x_scale - 0.5f;
      const float gy = (static_cast<float>(j) + 0.5f) * y_scale - 0.5f;

      const int gxi = static_cast<int>(std::floor(gx));
      const int gyi = static_cast<int>(std::floor(gy));

      const float tx = gx - static_cast<float>(gxi);
      const float ty = gy - static_cast<float>(gyi);

      const float c00 = at(gxi, gyi);
      const float c10 = at(gxi + 1, gyi);
      const float c01 = at(gxi, gyi + 1);
      const float c11 = at(gxi + 1, gyi + 1);

      const float top = (1.f - tx) * c00 + tx * c10;
      const float bot = (1.f - tx) * c01 + tx * c11;

      dst[static_cast<size_t>(j * dst_w + i)] = (1.f - ty) * top + ty * bot;
    }
  }
  return dst;
}

inline float cubic_interpolate(float p0, float p1, float p2, float p3, float t)
{
  const float a = -0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3;
  const float b = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
  const float c = -0.5f * p0 + 0.5f * p2;
  const float d = p1;
  return a * t * t * t + b * t * t + c * t + d;
}

inline std::vector<float> resample_bicubic_array(const std::vector<float> &src,
                                                 int src_w,
                                                 int src_h,
                                                 int dst_w,
                                                 int dst_h)
{
  if (src.empty() || src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
    return std::vector<float>(static_cast<size_t>(dst_w * dst_h), 0.f);

  auto at = [&](int x, int y) -> float
  {
    x = std::clamp(x, 0, src_w - 1);
    y = std::clamp(y, 0, src_h - 1);
    return src[static_cast<size_t>(y * src_w + x)];
  };

  std::vector<float> dst(static_cast<size_t>(dst_w * dst_h));
  const float x_scale = static_cast<float>(src_w) / static_cast<float>(dst_w);
  const float y_scale = static_cast<float>(src_h) / static_cast<float>(dst_h);

  for (int j = 0; j < dst_h; ++j)
  {
    for (int i = 0; i < dst_w; ++i)
    {
      const float gx = (static_cast<float>(i) + 0.5f) * x_scale - 0.5f;
      const float gy = (static_cast<float>(j) + 0.5f) * y_scale - 0.5f;

      const int gxi = static_cast<int>(std::floor(gx));
      const int gyi = static_cast<int>(std::floor(gy));

      const float tx = gx - static_cast<float>(gxi);
      const float ty = gy - static_cast<float>(gyi);

      float rows[4];
      for (int m = 0; m < 4; ++m)
      {
        const int row_y = gyi - 1 + m;
        rows[m] = cubic_interpolate(at(gxi - 1, row_y),
                                    at(gxi, row_y),
                                    at(gxi + 1, row_y),
                                    at(gxi + 2, row_y),
                                    tx);
      }

      dst[static_cast<size_t>(j * dst_w + i)] = cubic_interpolate(rows[0],
                                                                  rows[1],
                                                                  rows[2],
                                                                  rows[3],
                                                                  ty);
    }
  }
  return dst;
}

MetaWidget *render_array(AbstractAttribute &abstract_attr,
                         const RowContext &,
                         QWidget *parent)
{
  auto             &attr = static_cast<Attribute<meta::Array> &>(abstract_attr);
  std::string       widget_type = meta::common::widget_type(attr);
  const std::string label_txt = meta::common::label(attr);

  MetaWidget *widget = make_meta_widget_vbox(parent);
  auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

  if (widget_type.empty()) widget_type = "ArrayEditor";

  if (widget_type == "None")
  {
    return nullptr;
  }
  else if (widget_type == "ArrayEditor")
  {
    int canvas_w = 128;
    int canvas_h = 128;

    if (const auto *val = attr.metadata().try_value<int>(meta::keys::ui::width))
      canvas_w = *val;
    if (const auto *val = attr.metadata().try_value<int>(
            meta::keys::ui::height))
      canvas_h = *val;

    auto *canvas = new ArrayCanvas(label_txt, canvas_w, canvas_h, widget);
    layout->addWidget(canvas);

    widget->set_sync_from_model(
        [canvas, widget, &attr]()
        {
          if (widget->is_editing()) return;

          auto const        &arr = attr.value();
          std::vector<float> data = arr.vector;

          data = resample_bilinear_array(data,
                                         arr.shape.x,
                                         arr.shape.y,
                                         canvas->get_field_width(),
                                         canvas->get_field_height());
          canvas->set_field_data(data);
        });

    widget->sync_widget_from_model();

    meta::DataProvider data_provider;
    if (const auto *mp = attr.metadata().find(meta::keys::ui::data_provider))
      if (const auto *dp = mp->try_cast<meta::Attribute<meta::DataProvider>>())
        data_provider = dp->value();

    if (data_provider)
    {
      try
      {
        auto data = data_provider();
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

    QObject::connect(canvas,
                     &ArrayCanvas::value_changed,
                     widget,
                     [&attr, canvas, widget]()
                     {
                       Q_EMIT widget->edit_started();

                       auto const &cdata = canvas->get_field_data();
                       auto       &arr = attr.value();
                       arr.vector = resample_bicubic_array(
                           cdata,
                           canvas->get_field_width(),
                           canvas->get_field_height(),
                           arr.shape.x,
                           arr.shape.y);

                       Q_EMIT widget->value_changed();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(canvas,
                     &ArrayCanvas::edit_ended,
                     widget,
                     [&attr, canvas, widget]()
                     {
                       auto const &cdata = canvas->get_field_data();
                       auto       &arr = attr.value();
                       arr.vector = resample_bicubic_array(
                           cdata,
                           canvas->get_field_width(),
                           canvas->get_field_height(),
                           arr.shape.x,
                           arr.shape.y);

                       Q_EMIT widget->edit_ended();
                       attr.value_changed.notify(attr.value());
                     });
  }
  else
  {
    layout->addWidget(
        make_error_widget(&attr, "unsupported widget type", widget));
  }

  widget->connection_ = attr.value_changed.subscribe(
      [widget](meta::Array) { widget->sync_widget_from_model(); });

  return widget;
}
#endif

} // namespace

void register_stock_misc(DesignRegistry &registry)
{
  registry.add(kDesignName,
               std::type_index(typeid(std::vector<float>)),
               kAnyWidgetType,
               render_vec_float);

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
  registry.add(kDesignName,
               std::type_index(typeid(meta::ColorGradient)),
               kAnyWidgetType,
               render_color_gradient);
#endif

#ifdef META_ENABLE_ARRAY_TYPES
  registry.add(kDesignName,
               std::type_index(typeid(meta::Array)),
               kAnyWidgetType,
               render_array);
#endif
}

} // namespace meta::qt::stock
