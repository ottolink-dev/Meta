/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <vector>

#include "meta_common.hpp"
#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widgets/array_canvas.hpp"
#include "meta_qt/widgets/points_canvas.hpp" // For ImageData definition

#include "meta/ext/array/array.hpp"
#include "meta/metadata/keys.hpp"

namespace meta::qt
{

// =====================================
// Resampling Helpers
// =====================================

inline std::vector<float> resample_bilinear_array(const std::vector<float> &src,
                                                  int src_w,
                                                  int src_h,
                                                  int dst_w,
                                                  int dst_h)
{
  if (src.empty() || src_w <= 0 || src_h <= 0)
    return std::vector<float>(static_cast<size_t>(dst_w * dst_h), 0.f);

  std::vector<float> dst(static_cast<size_t>(dst_w * dst_h));
  const float x_scale = static_cast<float>(src_w) / static_cast<float>(dst_w);
  const float y_scale = static_cast<float>(src_h) / static_cast<float>(dst_h);

  for (int j = 0; j < dst_h; ++j)
  {
    for (int i = 0; i < dst_w; ++i)
    {
      const float sx = (static_cast<float>(i) + 0.5f) * x_scale - 0.5f;
      const float sy = (static_cast<float>(j) + 0.5f) * y_scale - 0.5f;

      const int x0 = std::clamp(static_cast<int>(std::floor(sx)), 0, src_w - 1);
      const int x1 = std::clamp(x0 + 1, 0, src_w - 1);
      const int y0 = std::clamp(static_cast<int>(std::floor(sy)), 0, src_h - 1);
      const int y1 = std::clamp(y0 + 1, 0, src_h - 1);

      const float tx = sx - std::floor(sx);
      const float ty = sy - std::floor(sy);

      const float v00 = src[static_cast<size_t>(y0 * src_w + x0)];
      const float v10 = src[static_cast<size_t>(y0 * src_w + x1)];
      const float v01 = src[static_cast<size_t>(y1 * src_w + x0)];
      const float v11 = src[static_cast<size_t>(y1 * src_w + x1)];

      dst[static_cast<size_t>(j * dst_w + i)] = (1.f - ty) * ((1.f - tx) * v00 +
                                                              tx * v10) +
                                                ty * ((1.f - tx) * v01 +
                                                      tx * v11);
    }
  }
  return dst;
}

inline float cubic_hermite_array(float a, float b, float c, float d, float t)
{
  const float A = -0.5f * a + 1.5f * b - 1.5f * c + 0.5f * d;
  const float B = a - 2.5f * b + 2.0f * c - 0.5f * d;
  const float C = -0.5f * a + 0.5f * c;
  const float D = b;
  return ((A * t + B) * t + C) * t + D;
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
      const float sx = (static_cast<float>(i) + 0.5f) * x_scale - 0.5f;
      const float sy = (static_cast<float>(j) + 0.5f) * y_scale - 0.5f;

      const int   x0 = static_cast<int>(std::floor(sx));
      const int   y0 = static_cast<int>(std::floor(sy));
      const float tx = sx - static_cast<float>(x0);
      const float ty = sy - static_cast<float>(y0);

      float rows[4];
      for (int r = -1; r <= 2; ++r)
        rows[r + 1] = cubic_hermite_array(at(x0 - 1, y0 + r),
                                          at(x0, y0 + r),
                                          at(x0 + 1, y0 + r),
                                          at(x0 + 2, y0 + r),
                                          tx);

      dst[static_cast<size_t>(j * dst_w + i)] = cubic_hermite_array(rows[0],
                                                                    rows[1],
                                                                    rows[2],
                                                                    rows[3],
                                                                    ty);
    }
  }
  return dst;
}

// ---------------------------------------------------------------------------
// WidgetRenderer<meta::Array>
// ---------------------------------------------------------------------------

template <> struct WidgetRenderer<meta::Array>
{
  static MetaWidget *render(Attribute<meta::Array> &attr, QWidget *parent)
  {
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

      if (const auto *val = attr.metadata().try_value<int>(
              meta::keys::ui::width))
        canvas_w = *val;
      if (const auto *val = attr.metadata().try_value<int>(
              meta::keys::ui::height))
        canvas_h = *val;

      auto *canvas = new ArrayCanvas(label_txt, canvas_w, canvas_h, widget);
      layout->addWidget(canvas);

      // Sync logic from model to widget
      widget->set_sync_from_model(
          [canvas, widget, &attr]()
          {
            // R1 sync contract: never clobber a live-edited canvas. During a
            // paint gesture the model is refreshed FROM the canvas; a host
            // recomputing synchronously on value_changed would otherwise
            // resample the stale model back over the in-progress stroke.
            if (widget->is_editing()) return;

            auto const        &arr = attr.value();
            std::vector<float> data = arr.vector;

            // Amplitude semantics: the canvas is a raw [0,1] surface (draw_at
            // and colormap both clamp to it), and the model is expected to
            // hold values in that same unit range (e.g. Hesiod's Brush node
            // treats it as normalized height). Do NOT min/max-stretch the
            // model into [0,1] here: that would make canvas and model
            // disagree on units, and since live-edit/edit_ended write the
            // canvas straight back into the model (no inverse transform),
            // every gesture after a sync would re-upload the display-
            // stretched amplitude, inflating the painting on each round
            // trip. Displaying model values directly keeps canvas and model
            // in the same units, so repeated sync/edit cycles are
            // amplitude-stable; out-of-range model data simply displays
            // saturated (clamped) rather than being silently rescaled.
            data = resample_bilinear_array(data,
                                           arr.shape.x,
                                           arr.shape.y,
                                           canvas->get_field_width(),
                                           canvas->get_field_height());
            canvas->set_field_data(data);
          });

      widget->sync_widget_from_model();

      // Background image DataProvider support
      meta::DataProvider data_provider;
      if (const auto *mp = attr.metadata().find(meta::keys::ui::data_provider))
        if (const auto
                *dp = mp->try_cast<meta::Attribute<meta::DataProvider>>())
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
          // a faulty host provider must not crash
        }
      }

      // Live edits
      QObject::connect(canvas,
                       &ArrayCanvas::value_changed,
                       widget,
                       [&attr, canvas, widget]()
                       {
                         // mark editing FIRST so the sync-from-model callback
                         // is inert for the rest of the gesture
                         Q_EMIT widget->edit_started();

                         // canvas -> model BEFORE announcing the change: hosts
                         // may recompute synchronously on value_changed and
                         // must see the fresh stroke, not the previous state
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

      // Committed edits
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
};

} // namespace meta::qt
