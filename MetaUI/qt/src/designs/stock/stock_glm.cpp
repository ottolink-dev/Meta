/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "stock_internal.hpp"

#ifdef META_ENABLE_GLM_TYPES

#include <cmath>
#include <random>

#include <QCheckBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <glm/glm.hpp>

#include "meta/core/data_provider.hpp"
#include "meta_common.hpp"
#include "meta_qt/designs/stock/stock.hpp"
#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widgets/points_canvas.hpp"
#include "meta_qt/widgets/power_of_two_spin_box.hpp"
#include "meta_qt/widgets/range_bar.hpp"
#include "meta_qt/widgets/responsive_box.hpp"
#include "meta_qt/widgets/slider_float.hpp"
#include "meta_qt/widgets/vector_canvas.hpp"
#include "meta_qt/widgets/xy_canvas.hpp"

namespace meta::qt::stock
{

namespace
{

inline int ceil_power_of_two(int v)
{
  if (v <= 1) return 1;
  int p = 1;
  while (p < v)
    p <<= 1;
  return p;
}

MetaWidget *render_ivec2(AbstractAttribute &abstract_attr,
                         const RowContext &,
                         QWidget *parent)
{
  auto             &attr = static_cast<Attribute<glm::ivec2> &>(abstract_attr);
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

  if (widget_type == "None")
  {
    return nullptr;
  }
  else if (widget_type == "Input")
  {
    if (!label_txt.empty())
    {
      layout->addWidget(new QLabel(QString::fromStdString(label_txt), widget));
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
          {
            QSignalBlocker b(spinbox_x);
            int            x = std::clamp(value.x, min, max);
            if (power_of_two) x = ceil_power_of_two(x);
            spinbox_x->setValue(x);
          }

          {
            QSignalBlocker b(spinbox_y);
            int            y = std::clamp(value.y, min, max);
            if (keep_aspect)
              y = int(std::lround(double(spinbox_x->value()) / aspect_ratio));
            else if (power_of_two)
              y = ceil_power_of_two(y);
            spinbox_y->setValue(y);
          }
        });

    QObject::connect(spinbox_x,
                     qOverload<int>(&QSpinBox::valueChanged),
                     widget,
                     [&attr,
                      &value,
                      widget,
                      spinbox_x,
                      spinbox_y,
                      min,
                      max,
                      keep_aspect,
                      aspect_ratio](int v)
                     {
                       int x = std::clamp(v, min, max);
                       int y = value.y;

                       if (keep_aspect)
                       {
                         y = int(std::lround(double(x) / aspect_ratio));
                         QSignalBlocker blocker(spinbox_y);
                         spinbox_y->setValue(y);
                       }

                       {
                         QSignalBlocker blocker(spinbox_x);
                         spinbox_x->setValue(x);
                       }

                       attr.set_from_any(glm::ivec2{x, y});

                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
                     });

    QObject::connect(spinbox_y,
                     qOverload<int>(&QSpinBox::valueChanged),
                     widget,
                     [&attr, &value, widget, spinbox_y, min, max](int v)
                     {
                       int y = std::clamp(v, min, max);
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
  else
  {
    layout->addWidget(
        make_error_widget(&attr, "unsupported widget type", widget));
  }

  widget->connection_ = attr.value_changed.subscribe(
      [widget](glm::ivec2) { widget->sync_widget_from_model(); });

  return widget;
}

MetaWidget *render_vec2(AbstractAttribute &abstract_attr,
                        const RowContext &,
                        QWidget *parent)
{
  auto             &attr = static_cast<Attribute<glm::vec2> &>(abstract_attr);
  std::string       widget_type = meta::common::widget_type(attr);
  const std::string label_txt = meta::common::label(attr);
  const std::string format = meta::common::format(attr);
  const float       min = meta::common::min<float>(attr);
  const float       max = meta::common::max<float>(attr);
  const float       step = meta::common::step<float>(attr);
  const bool        show_grid = meta::common::try_get<bool>(attr,
                                                     "ui.show_grid",
                                                     true);

  const std::string x_label = meta::common::try_get<std::string>(attr,
                                                                 "ui.label_x",
                                                                 "x");
  const std::string y_label = meta::common::try_get<std::string>(attr,
                                                                 "ui.label_y",
                                                                 "y");
  const int         decimals = meta::common::try_get_format_decimals(format);

  bool locked_xy = false;
  if (const auto *p = attr.state().try_value<bool>(
          meta::keys::state::locked_xy))
    locked_xy = *p;

  glm::vec2 &value = attr.value();

  MetaWidget *widget = make_meta_widget_vbox(parent);
  auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

  if (widget_type.empty()) widget_type = "Input";

  if (!label_txt.empty())
    layout->addWidget(new QLabel(QString::fromStdString(label_txt), widget));

  if (widget_type == "None")
  {
    return nullptr;
  }
  else if (widget_type == "Input")
  {
    auto *row = new QHBoxLayout();

    auto *spinbox_x = new QDoubleSpinBox(widget);
    auto *spinbox_y = new QDoubleSpinBox(widget);

    for (auto *sp : {spinbox_x, spinbox_y})
    {
      sp->setRange(min, max);
      sp->setSingleStep(step);
      sp->setDecimals(decimals);
    }

    spinbox_x->setValue(std::clamp(value.x, min, max));
    spinbox_y->setValue(std::clamp(value.y, min, max));

    row->addWidget(spinbox_x);
    row->addWidget(spinbox_y);
    layout->addLayout(row);

    widget->set_sync_from_model(
        [&value, spinbox_x, spinbox_y, min, max]()
        {
          {
            QSignalBlocker b(spinbox_x);
            spinbox_x->setValue(std::clamp(value.x, min, max));
          }
          {
            QSignalBlocker b(spinbox_y);
            spinbox_y->setValue(std::clamp(value.y, min, max));
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
                       attr.set_from_any(glm::vec2{x, value.y});
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
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
                       attr.set_from_any(glm::vec2{value.x, y});
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
                     });
  }
  else if (widget_type == "XYCanvas")
  {
    const float min_x = meta::common::try_get<float>(attr,
                                                     meta::keys::ui::min_x,
                                                     min);
    const float max_x = meta::common::try_get<float>(attr,
                                                     meta::keys::ui::max_x,
                                                     max);
    const float min_y = meta::common::try_get<float>(attr,
                                                     meta::keys::ui::min_y,
                                                     min);
    const float max_y = meta::common::try_get<float>(attr,
                                                     meta::keys::ui::max_y,
                                                     max);

    auto *canvas =
        new XYCanvas(value, min_x, max_x, min_y, max_y, show_grid, widget);
    layout->addWidget(canvas);

    auto *btn_row = new QHBoxLayout();
    auto *center_btn = new QPushButton(QObject::tr("Center"), widget);
    auto *random_btn = new QPushButton(QObject::tr("Random"), widget);

    for (auto *btn : {center_btn, random_btn})
    {
      btn->setFixedHeight(22);
      btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      btn_row->addWidget(btn);
    }
    layout->addLayout(btn_row);

    widget->set_sync_from_model(
        [&value, canvas]()
        {
          QSignalBlocker blocker(canvas);
          canvas->set_value(value);
        });

    QObject::connect(canvas,
                     &XYCanvas::value_changed,
                     widget,
                     [widget, &attr](glm::vec2)
                     {
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(canvas,
                     &XYCanvas::drag_ended,
                     widget,
                     [widget, &attr](glm::vec2)
                     {
                       Q_EMIT widget->edit_ended();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(center_btn,
                     &QPushButton::clicked,
                     widget,
                     [&attr, min, max, canvas, widget]()
                     {
                       const glm::vec2 center = {(min + max) * 0.5f,
                                                 (min + max) * 0.5f};
                       attr.set_from_any(center);
                       canvas->set_value(center);
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
                     });

    QObject::connect(random_btn,
                     &QPushButton::clicked,
                     widget,
                     [&attr, min, max, canvas, widget]()
                     {
                       static std::mt19937 rng{std::random_device{}()};
                       std::uniform_real_distribution<float> dist_x(min, max);
                       std::uniform_real_distribution<float> dist_y(min, max);
                       const glm::vec2 rv = {dist_x(rng), dist_y(rng)};
                       attr.set_from_any(rv);
                       canvas->set_value(rv);
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
                     });
  }
  else if (widget_type == "RangeBar")
  {
    bool is_active = true;
    if (const auto *p = attr.state().try_value<bool>(meta::keys::state::active))
      is_active = *p;

    glm::vec2 last_active_value = is_active ? value : glm::vec2{min, max};

    auto *bar = new RangeBar(value, min, max, decimals, widget);

    meta::DataProvider range_provider;
    if (const auto *mp = attr.metadata().find(meta::keys::ui::data_provider))
      if (const auto *dp = mp->try_cast<meta::Attribute<meta::DataProvider>>())
        range_provider = dp->value();

    if (range_provider)
    {
      try
      {
        auto data = range_provider();
        if (auto histogram = data.get<HistogramData>())
          bar->set_histogram(histogram->x, histogram->y);
        else
          bar->set_histogram({}, {});
      }
      catch (...)
      {
      }
    }

    auto *btn_row = new QHBoxLayout();
    auto *toggle_btn = new QPushButton(widget);
    auto *reset_btn = new QPushButton(QObject::tr("Full"), widget);
    auto *center_btn = new QPushButton(QObject::tr("Center"), widget);
    auto *unit_btn = new QPushButton(QObject::tr("[0, 1]"), widget);

    toggle_btn->setCheckable(true);
    toggle_btn->setChecked(is_active);
    toggle_btn->setText(is_active ? QObject::tr("On") : QObject::tr("Off"));
    toggle_btn->setFixedHeight(22);
    toggle_btn->setFixedWidth(40);

    for (auto *btn : {reset_btn, center_btn, unit_btn})
    {
      btn->setFixedHeight(22);
      btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    btn_row->addWidget(toggle_btn);
    btn_row->addSpacing(4);
    btn_row->addWidget(reset_btn);
    btn_row->addWidget(center_btn);
    btn_row->addWidget(unit_btn);

    layout->addWidget(bar);
    layout->addLayout(btn_row);

    auto set_active =
        [&value, bar, reset_btn, center_btn, unit_btn, min, max](bool active)
    {
      const bool full_domain = value.x <= min && value.y >= max;
      bar->setEnabled(active);
      reset_btn->setEnabled(active && !full_domain);
      center_btn->setEnabled(active && value.y - value.x < max - min);
      unit_btn->setEnabled(active);
    };

    set_active(is_active);

    widget->set_sync_from_model(
        [&value, &attr, bar, toggle_btn, set_active, widget, range_provider]()
        {
          bool active = true;
          if (const auto *p = attr.state().try_value<bool>(
                  meta::keys::state::active))
            active = *p;

          set_active(active);

          {
            QSignalBlocker b(toggle_btn);
            toggle_btn->setChecked(active);
            toggle_btn->setText(active ? QObject::tr("On")
                                       : QObject::tr("Off"));
          }

          {
            QSignalBlocker b(bar);
            bar->set_value(value);
            bar->update();
          }

          if (range_provider && !widget->is_editing())
          {
            try
            {
              auto data = range_provider();
              if (auto histogram = data.get<HistogramData>())
                bar->set_histogram(histogram->x, histogram->y);
              else
                bar->set_histogram({}, {});
            }
            catch (...)
            {
            }
          }
        });

    QObject::connect(
        toggle_btn,
        &QPushButton::toggled,
        widget,
        [&value,
         &attr,
         bar,
         toggle_btn,
         set_active,
         widget,
         lav = last_active_value](bool active) mutable
        {
          toggle_btn->setText(active ? QObject::tr("On") : QObject::tr("Off"));

          if (active)
          {
            attr.set_from_any(lav);
            bar->set_value(lav);
          }
          else
          {
            lav = value;
            attr.set_from_any(glm::vec2{-1.f, 0.f});
            bar->set_value({-1.f, 0.f});
          }

          if (auto *p = attr.state().try_value<bool>(meta::keys::state::active))
            *p = active;

          set_active(active);

          Q_EMIT widget->edit_started();
          Q_EMIT widget->value_changed();
          Q_EMIT widget->edit_ended();
        });

    QObject::connect(bar,
                     &RangeBar::value_changed,
                     widget,
                     [widget, set_active](glm::vec2)
                     {
                       set_active(true);
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                     });

    QObject::connect(bar,
                     &RangeBar::drag_ended,
                     widget,
                     [widget](glm::vec2) { Q_EMIT widget->edit_ended(); });

    QObject::connect(reset_btn,
                     &QPushButton::clicked,
                     widget,
                     [&attr, min, max, bar, widget, set_active]()
                     {
                       bar->set_value({min, max});
                       attr.set_from_any(glm::vec2{min, max});
                       set_active(true);
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
                     });

    QObject::connect(
        center_btn,
        &QPushButton::clicked,
        widget,
        [&value, &attr, min, max, bar, widget, set_active]()
        {
          const float span = value.y - value.x;
          const float mid = (min + max) * 0.5f;
          const float lo = std::clamp(mid - span * 0.5f, min, max - span);
          bar->set_value({lo, lo + span});
          attr.set_from_any(glm::vec2{lo, lo + span});
          set_active(true);
          Q_EMIT widget->edit_started();
          Q_EMIT widget->value_changed();
          Q_EMIT widget->edit_ended();
        });

    QObject::connect(unit_btn,
                     &QPushButton::clicked,
                     widget,
                     [&attr, min, max, bar, widget, set_active]()
                     {
                       const float lo = std::clamp(0.f, min, max);
                       const float hi = std::clamp(1.f, min, max);
                       bar->set_value({lo, hi});
                       attr.set_from_any(glm::vec2{lo, hi});
                       set_active(true);
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
                     });
  }
  else if (widget_type == "VectorEditor")
  {
    auto *canvas = new VectorCanvas(value, max, locked_xy, widget);

    auto *canvas_row = new QHBoxLayout();
    canvas_row->addStretch();
    canvas_row->addWidget(canvas);
    canvas_row->addStretch();
    layout->addLayout(canvas_row);

    auto *form = new QFormLayout();
    form->setContentsMargins(0, 2, 0, 2);
    form->setSpacing(3);

    auto *mag_spin = new QDoubleSpinBox(widget);
    mag_spin->setRange(0.0, double(max));
    mag_spin->setDecimals(decimals);
    mag_spin->setSingleStep(double(max) / 100.0);
    mag_spin->setValue(double(canvas->magnitude()));
    mag_spin->setFixedHeight(22);

    auto *angle_spin = new QDoubleSpinBox(widget);
    angle_spin->setRange(-360.0, 360.0);
    angle_spin->setDecimals(1);
    angle_spin->setSingleStep(1.0);
    angle_spin->setSuffix("°");
    angle_spin->setValue(double(canvas->angle_deg()));
    angle_spin->setEnabled(locked_xy);
    angle_spin->setFixedHeight(22);

    form->addRow(QObject::tr("Magnitude"), mag_spin);
    form->addRow(QObject::tr("Angle"), angle_spin);
    layout->addLayout(form);

    auto *lock_row = new QHBoxLayout();
    auto *lock_cb = new QCheckBox(QObject::tr("Isotropic  (kx = ky)"), widget);
    lock_cb->setChecked(locked_xy);
    lock_row->addStretch();
    lock_row->addWidget(lock_cb);
    layout->addLayout(lock_row);

    widget->set_sync_from_model(
        [&attr, &value, canvas, mag_spin, angle_spin, lock_cb]()
        {
          float mag = glm::length(value);
          float deg = (mag > 1e-6f)
                          ? std::atan2(value.y, value.x) * 180.f / float(M_PI)
                          : 45.f;
          bool  stored_locked_state = false;
          if (const auto *p = attr.state().try_value<bool>(
                  meta::keys::state::locked_xy))
            stored_locked_state = *p;

          {
            QSignalBlocker b(canvas);
            canvas->set_locked(stored_locked_state);
            canvas->set_magnitude(mag);
            canvas->set_angle_deg(deg);
          }
          {
            QSignalBlocker b(mag_spin);
            mag_spin->setValue(mag);
          }
          {
            QSignalBlocker b(angle_spin);
            angle_spin->setValue(deg);
            angle_spin->setEnabled(!stored_locked_state);
          }
          {
            QSignalBlocker b(lock_cb);
            lock_cb->setChecked(stored_locked_state);
          }
        });

    QObject::connect(canvas,
                     &VectorCanvas::magnitude_changed,
                     widget,
                     [&attr, mag_spin](float mag)
                     {
                       QSignalBlocker b(mag_spin);
                       mag_spin->setValue(double(mag));
                     });

    QObject::connect(canvas,
                     &VectorCanvas::angle_changed,
                     widget,
                     [&attr, angle_spin](float deg)
                     {
                       QSignalBlocker b(angle_spin);
                       angle_spin->setValue(double(deg));
                     });

    QObject::connect(mag_spin,
                     qOverload<double>(&QDoubleSpinBox::valueChanged),
                     widget,
                     [canvas](double v) { canvas->set_magnitude(float(v)); });

    QObject::connect(angle_spin,
                     qOverload<double>(&QDoubleSpinBox::valueChanged),
                     widget,
                     [canvas](double v) { canvas->set_angle_deg(float(v)); });

    QObject::connect(lock_cb,
                     &QCheckBox::toggled,
                     widget,
                     [&attr, canvas, angle_spin](bool checked)
                     {
                       canvas->set_locked(checked);
                       angle_spin->setEnabled(!checked);
                       attr.state()
                           .try_add(meta::keys::state::locked_xy, checked)
                           ->value() = checked;
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(canvas,
                     &VectorCanvas::value_changed,
                     widget,
                     [&attr, widget](glm::vec2)
                     {
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(canvas,
                     &VectorCanvas::drag_ended,
                     widget,
                     [&attr, widget](glm::vec2)
                     {
                       Q_EMIT widget->edit_ended();
                       attr.value_changed.notify(attr.value());
                     });

    auto spinbox_commit = [&attr, widget]()
    {
      Q_EMIT widget->edit_ended();
      attr.value_changed.notify(attr.value());
    };

    QObject::connect(mag_spin,
                     &QDoubleSpinBox::editingFinished,
                     widget,
                     spinbox_commit);
    QObject::connect(angle_spin,
                     &QDoubleSpinBox::editingFinished,
                     widget,
                     spinbox_commit);

    QObject::connect(lock_cb,
                     &QCheckBox::toggled,
                     widget,
                     [&attr, widget](bool)
                     {
                       Q_EMIT widget->edit_ended();
                       attr.value_changed.notify(attr.value());
                     });
  }
  else if (widget_type == "LinkedSliders")
  {
    auto *row = new QHBoxLayout();
    row->setSpacing(4);

    auto *slider_x = new SliderFloat(x_label,
                                     value.x,
                                     min,
                                     max,
                                     false,
                                     format,
                                     false,
                                     widget);
    auto *slider_y = new SliderFloat(y_label,
                                     value.y,
                                     min,
                                     max,
                                     false,
                                     format,
                                     false,
                                     widget);

    slider_x->set_value(value.x);
    slider_y->set_value(value.y);

    auto *lock_btn = new QPushButton(QObject::tr("X=Y"), widget);
    lock_btn->setCheckable(true);
    lock_btn->setChecked(locked_xy);
    lock_btn->setFixedWidth(36);
    lock_btn->setFixedHeight(slider_x->sizeHint().height());
    lock_btn->setToolTip(QObject::tr("Lock X = Y"));

    auto update_lock_style = [lock_btn](bool locked)
    {
      lock_btn->setProperty("locked", locked);
      lock_btn->setStyleSheet(locked ? "font-weight: bold;"
                                     : "font-weight: normal;");
    };
    update_lock_style(locked_xy);

    if (locked_xy)
    {
      value.y = value.x;
      slider_y->set_value(value.x);
      slider_y->setEnabled(false);
    }

    auto *pair = new ResponsiveBox(widget);
    pair->set_spacing(4);
    pair->add_widget(slider_x, 1);
    pair->add_widget(slider_y, 1);

    row->addWidget(pair, 1);
    row->addWidget(lock_btn, 0);
    layout->addLayout(row);

    widget->set_sync_from_model(
        [&attr, widget_type, &value, slider_x, slider_y, lock_btn]()
        {
          {
            QSignalBlocker b(slider_x);
            slider_x->set_value(value.x);
          }
          {
            QSignalBlocker b(slider_y);
            slider_y->set_value(value.y);
          }
          bool stored_locked_state = false;
          if (const auto *p = attr.state().try_value<bool>(
                  meta::keys::state::locked_xy))
            stored_locked_state = *p;

          {
            QSignalBlocker b(lock_btn);
            lock_btn->setChecked(stored_locked_state);
          }

          slider_y->setEnabled(!lock_btn->isChecked());
        });

    QObject::connect(slider_x,
                     &SliderFloat::value_changed,
                     widget,
                     [&attr, &value, slider_x, slider_y, lock_btn, widget]()
                     {
                       value.x = slider_x->get_value();
                       if (lock_btn->isChecked())
                       {
                         value.y = value.x;
                         QSignalBlocker b(slider_y);
                         slider_y->set_value(value.x);
                       }
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(slider_x,
                     &SliderFloat::edit_ended,
                     widget,
                     [&attr, &value, slider_x, widget]()
                     {
                       value.x = slider_x->get_value();
                       Q_EMIT widget->edit_ended();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(slider_y,
                     &SliderFloat::value_changed,
                     widget,
                     [&attr, &value, slider_y, widget]()
                     {
                       value.y = slider_y->get_value();
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(slider_y,
                     &SliderFloat::edit_ended,
                     widget,
                     [&attr, &value, slider_y, widget]()
                     {
                       value.y = slider_y->get_value();
                       Q_EMIT widget->edit_ended();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(lock_btn,
                     &QPushButton::toggled,
                     widget,
                     [&attr,
                      &value,
                      slider_x,
                      slider_y,
                      lock_btn,
                      update_lock_style,
                      widget](bool checked)
                     {
                       update_lock_style(checked);
                       slider_y->setEnabled(!checked);

                       if (checked)
                       {
                         value.y = value.x;
                         QSignalBlocker b(slider_y);
                         slider_y->set_value(value.x);
                       }

                       attr.state()
                           .try_add(meta::keys::state::locked_xy, checked)
                           ->value() = checked;

                       attr.value_changed.notify(attr.value());
                       Q_EMIT widget->edit_ended();
                     });
  }
  else
  {
    layout->addWidget(
        make_error_widget(&attr, "unsupported widget type", widget));
  }

  widget->connection_ = attr.value_changed.subscribe(
      [widget](glm::vec2) { widget->sync_widget_from_model(); });

  return widget;
}

MetaWidget *render_vec3(AbstractAttribute &abstract_attr,
                        const RowContext &,
                        QWidget *parent)
{
  auto             &attr = static_cast<Attribute<glm::vec3> &>(abstract_attr);
  std::string       widget_type = meta::common::widget_type(attr);
  const std::string label_txt = meta::common::label(attr);
  const std::string format = meta::common::format(attr);
  const float       min = meta::common::min<float>(attr);
  const float       max = meta::common::max<float>(attr);
  const float       step = meta::common::step<float>(attr);
  const int         decimals = meta::common::try_get_format_decimals(format);

  glm::vec3 &value = attr.value();

  MetaWidget *widget = make_meta_widget_vbox(parent);
  auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

  if (widget_type.empty()) widget_type = "Input";

  if (!label_txt.empty())
    layout->addWidget(new QLabel(QString::fromStdString(label_txt), widget));

  if (widget_type == "None")
  {
    return nullptr;
  }
  else if (widget_type == "Input")
  {
    auto *row = new QHBoxLayout();
    auto *spinbox_x = new QDoubleSpinBox(widget);
    auto *spinbox_y = new QDoubleSpinBox(widget);
    auto *spinbox_z = new QDoubleSpinBox(widget);

    for (auto *sp : {spinbox_x, spinbox_y, spinbox_z})
    {
      sp->setRange(min, max);
      sp->setSingleStep(step);
      sp->setDecimals(decimals);
    }

    spinbox_x->setValue(std::clamp(value.x, min, max));
    spinbox_y->setValue(std::clamp(value.y, min, max));
    spinbox_z->setValue(std::clamp(value.z, min, max));

    row->addWidget(spinbox_x);
    row->addWidget(spinbox_y);
    row->addWidget(spinbox_z);
    layout->addLayout(row);

    widget->set_sync_from_model(
        [&value, spinbox_x, spinbox_y, spinbox_z, min, max]()
        {
          {
            QSignalBlocker b(spinbox_x);
            spinbox_x->setValue(std::clamp(value.x, min, max));
          }
          {
            QSignalBlocker b(spinbox_y);
            spinbox_y->setValue(std::clamp(value.y, min, max));
          }
          {
            QSignalBlocker b(spinbox_z);
            spinbox_z->setValue(std::clamp(value.z, min, max));
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
                       attr.set_from_any(glm::vec3{x, value.y, value.z});
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
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
                       attr.set_from_any(glm::vec3{value.x, y, value.z});
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
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
                       attr.set_from_any(glm::vec3{value.x, value.y, z});
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
                     });
  }
  else if (widget_type == "ColorPicker")
  {
    auto *button = new QPushButton(widget);
    button->setAutoFillBackground(true);

    auto update_color = [button](const glm::vec3 &color)
    {
      const int r = static_cast<int>(
          std::clamp(color.r * 255.0f, 0.0f, 255.0f));
      const int g = static_cast<int>(
          std::clamp(color.g * 255.0f, 0.0f, 255.0f));
      const int b = static_cast<int>(
          std::clamp(color.b * 255.0f, 0.0f, 255.0f));

      const QString style = QString("background-color: rgb(%1, %2, %3);"
                                    "border: 1px solid #555;"
                                    "border-radius: 4px;"
                                    "min-height: 24px;")
                                .arg(r)
                                .arg(g)
                                .arg(b);
      button->setStyleSheet(style);
    };

    update_color(value);
    layout->addWidget(button);

    widget->set_sync_from_model([&value, update_color]()
                                { update_color(value); });

    QObject::connect(button,
                     &QPushButton::clicked,
                     widget,
                     [&attr, &value, widget, update_color]()
                     {
                       const QColor initial_color = QColor::fromRgbF(
                           std::clamp(value.r, 0.0f, 1.0f),
                           std::clamp(value.g, 0.0f, 1.0f),
                           std::clamp(value.b, 0.0f, 1.0f));

                       const QColor color = QColorDialog::getColor(
                           initial_color,
                           widget,
                           "Select Color");

                       if (color.isValid())
                       {
                         const glm::vec3 new_color(color.redF(),
                                                   color.greenF(),
                                                   color.blueF());
                         attr.set_from_any(new_color);
                         update_color(new_color);
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       }
                     });
  }
  else
  {
    layout->addWidget(
        make_error_widget(&attr, "unsupported widget type", widget));
  }

  widget->connection_ = attr.value_changed.subscribe(
      [widget](glm::vec3) { widget->sync_widget_from_model(); });

  return widget;
}

MetaWidget *render_vec4(AbstractAttribute &abstract_attr,
                        const RowContext &,
                        QWidget *parent)
{
  auto             &attr = static_cast<Attribute<glm::vec4> &>(abstract_attr);
  std::string       widget_type = meta::common::widget_type(attr);
  const std::string label_txt = meta::common::label(attr);
  const std::string format = meta::common::format(attr);
  const float       min = meta::common::min<float>(attr);
  const float       max = meta::common::max<float>(attr);
  const float       step = meta::common::step<float>(attr);
  const int         decimals = meta::common::try_get_format_decimals(format);

  glm::vec4 &value = attr.value();

  MetaWidget *widget = make_meta_widget_vbox(parent);
  auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

  if (widget_type.empty()) widget_type = "Input";

  if (!label_txt.empty())
    layout->addWidget(new QLabel(QString::fromStdString(label_txt), widget));

  if (widget_type == "None")
  {
    return nullptr;
  }
  else if (widget_type == "Input")
  {
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
        [&value, spinbox_x, spinbox_y, spinbox_z, spinbox_w, min, max]()
        {
          {
            QSignalBlocker b(spinbox_x);
            spinbox_x->setValue(std::clamp(value.x, min, max));
          }
          {
            QSignalBlocker b(spinbox_y);
            spinbox_y->setValue(std::clamp(value.y, min, max));
          }
          {
            QSignalBlocker b(spinbox_z);
            spinbox_z->setValue(std::clamp(value.z, min, max));
          }
          {
            QSignalBlocker b(spinbox_w);
            spinbox_w->setValue(std::clamp(value.w, min, max));
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
                       attr.set_from_any(
                           glm::vec4{x, value.y, value.z, value.w});
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
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
                       attr.set_from_any(
                           glm::vec4{value.x, y, value.z, value.w});
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
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
                       attr.set_from_any(
                           glm::vec4{value.x, value.y, z, value.w});
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
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
                       attr.set_from_any(
                           glm::vec4{value.x, value.y, value.z, w});
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
                     });
  }
  else if (widget_type == "ColorPicker")
  {
    auto *button = new QPushButton(widget);
    button->setAutoFillBackground(true);

    auto update_color = [button](const glm::vec4 &color)
    {
      const int r = static_cast<int>(
          std::clamp(color.r * 255.0f, 0.0f, 255.0f));
      const int g = static_cast<int>(
          std::clamp(color.g * 255.0f, 0.0f, 255.0f));
      const int b = static_cast<int>(
          std::clamp(color.b * 255.0f, 0.0f, 255.0f));
      const float a = std::clamp(color.a, 0.0f, 1.0f);

      const QString style = QString("background-color: rgba(%1, %2, %3, %4);"
                                    "border: 1px solid #555;"
                                    "border-radius: 4px;"
                                    "min-height: 24px;")
                                .arg(r)
                                .arg(g)
                                .arg(b)
                                .arg(a);
      button->setStyleSheet(style);
    };

    update_color(value);
    layout->addWidget(button);

    widget->set_sync_from_model([&value, update_color]()
                                { update_color(value); });

    QObject::connect(button,
                     &QPushButton::clicked,
                     widget,
                     [&attr, &value, widget, update_color]()
                     {
                       const QColor initial_color = QColor::fromRgbF(
                           std::clamp(value.r, 0.0f, 1.0f),
                           std::clamp(value.g, 0.0f, 1.0f),
                           std::clamp(value.b, 0.0f, 1.0f),
                           std::clamp(value.a, 0.0f, 1.0f));

                       const QColor color = QColorDialog::getColor(
                           initial_color,
                           widget,
                           "Select Color",
                           QColorDialog::ShowAlphaChannel);

                       if (color.isValid())
                       {
                         const glm::vec4 new_color(color.redF(),
                                                   color.greenF(),
                                                   color.blueF(),
                                                   color.alphaF());
                         attr.set_from_any(new_color);
                         update_color(new_color);
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       }
                     });
  }
  else
  {
    layout->addWidget(
        make_error_widget(&attr, "unsupported widget type", widget));
  }

  widget->connection_ = attr.value_changed.subscribe(
      [widget](glm::vec4) { widget->sync_widget_from_model(); });

  return widget;
}

MetaWidget *render_vec_glm_vec3(AbstractAttribute &abstract_attr,
                                const RowContext &,
                                QWidget *parent)
{
  auto &attr = static_cast<Attribute<std::vector<glm::vec3>> &>(abstract_attr);
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

  if (widget_type == "None")
  {
    return nullptr;
  }
  else if (is_points || is_path)
  {
    if (!label_txt.empty())
      layout->addWidget(new QLabel(QString::fromStdString(label_txt), widget));

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

    meta::DataProvider points_provider;
    if (const auto *mp = attr.metadata().find(meta::keys::ui::data_provider))
      if (const auto *dp = mp->try_cast<meta::Attribute<meta::DataProvider>>())
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
      }
    }

    auto *toolbar = new QHBoxLayout();

    auto *clear_btn = new QPushButton(QObject::tr("Clear"), widget);
    clear_btn->setFixedHeight(22);
    clear_btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    toolbar->addWidget(clear_btn);

    auto *rand_btn = new QPushButton(QObject::tr("Randomize"), widget);
    rand_btn->setFixedHeight(22);
    rand_btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    toolbar->addWidget(rand_btn);

    auto *csv_btn = new QPushButton(QObject::tr("From CSV…"), widget);
    csv_btn->setFixedHeight(22);
    csv_btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    toolbar->addWidget(csv_btn);

    layout->addLayout(toolbar);

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

    QObject::connect(canvas,
                     &PointsCanvas::points_changed,
                     widget,
                     [&attr, widget]()
                     {
                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(canvas,
                     &PointsCanvas::drag_ended,
                     widget,
                     [&attr, widget]()
                     {
                       Q_EMIT widget->edit_ended();
                       attr.value_changed.notify(attr.value());
                     });

    QObject::connect(clear_btn,
                     &QPushButton::clicked,
                     canvas,
                     &PointsCanvas::clear_all);

    QObject::connect(rand_btn,
                     &QPushButton::clicked,
                     widget,
                     [&attr, &value, canvas]()
                     {
                       canvas->randomize(value.size());
                       attr.value_changed.notify(attr.value());
                     });

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
  else
  {
    layout->addWidget(
        make_error_widget(&attr, "unsupported widget type", widget));
  }

  widget->connection_ = attr.value_changed.subscribe(
      [widget](std::vector<glm::vec3>) { widget->sync_widget_from_model(); });

  return widget;
}

} // namespace

void register_stock_glm(DesignRegistry &registry)
{
  registry.add(kDesignName,
               std::type_index(typeid(glm::ivec2)),
               kAnyWidgetType,
               render_ivec2);

  const std::type_index type_vec2 = std::type_index(typeid(glm::vec2));
  registry.add(kDesignName, type_vec2, "Input", render_vec2);
  registry.add(kDesignName, type_vec2, "XYCanvas", render_vec2);
  registry.add(kDesignName, type_vec2, "VectorEditor", render_vec2);
  registry.add(kDesignName, type_vec2, "LinkedSliders", render_vec2);
  registry.add(kDesignName, type_vec2, "RangeBar", render_vec2);
  registry.add(kDesignName, type_vec2, kAnyWidgetType, render_vec2);

  const std::type_index type_vec3 = std::type_index(typeid(glm::vec3));
  registry.add(kDesignName, type_vec3, "Input", render_vec3);
  registry.add(kDesignName, type_vec3, "ColorPicker", render_vec3);
  registry.add(kDesignName, type_vec3, kAnyWidgetType, render_vec3);

  const std::type_index type_vec4 = std::type_index(typeid(glm::vec4));
  registry.add(kDesignName, type_vec4, "Input", render_vec4);
  registry.add(kDesignName, type_vec4, "ColorPicker", render_vec4);
  registry.add(kDesignName, type_vec4, kAnyWidgetType, render_vec4);

  const std::type_index type_vec_vec3 = std::type_index(
      typeid(std::vector<glm::vec3>));
  registry.add(kDesignName, type_vec_vec3, "PointsEditor", render_vec_glm_vec3);
  registry.add(kDesignName, type_vec_vec3, "PathEditor", render_vec_glm_vec3);
  registry.add(kDesignName, type_vec_vec3, kAnyWidgetType, render_vec_glm_vec3);
}

} // namespace meta::qt::stock

#endif
