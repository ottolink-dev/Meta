/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <random>

#include <QFormLayout>

#include "meta/core/data_provider.hpp"
#include "meta_qt/widgets/range_bar.hpp"
#include "meta_qt/widgets/responsive_box.hpp"
#include "meta_qt/widgets/vector_canvas.hpp"
#include "meta_qt/widgets/xy_canvas.hpp"

namespace meta::qt
{

template <> struct WidgetRenderer<glm::vec2>
{
  static MetaWidget *render(Attribute<glm::vec2> &attr, QWidget *parent)
  {
    std::string       widget_type = meta::common::widget_type(attr);
    const std::string label_txt = meta::common::label(attr);
    const std::string format = meta::common::format(attr);
    const float       min = meta::common::min<float>(attr);
    const float       max = meta::common::max<float>(attr);
    const float       step = meta::common::step<float>(attr);
    const bool        show_grid = meta::common::try_get<bool>(attr,
                                                       "ui.show_grid",
                                                       true);
    bool              locked_xy = meta::common::try_get<bool>(attr,
                                                 meta::keys::ui::locked_xy,
                                                 false);
    const std::string x_label = meta::common::try_get<std::string>(attr,
                                                                   "ui.label_x",
                                                                   "x");
    const std::string y_label = meta::common::try_get<std::string>(attr,
                                                                   "ui.label_y",
                                                                   "y");

    const int decimals = meta::common::try_get_format_decimals(format);

    // --- UI state management

    auto *state = attr.metadata().try_add(meta::keys::ui::state, true);
    state->metadata().try_add(meta::keys::ui::widget_type, "None");

    // either add with current input state 'locked_xy' or override
    // current 'locked_xy' with metadata
    locked_xy = state->metadata()
                    .try_add(widget_type + ".locked_xy", locked_xy)
                    ->value();

    // --- Generate widget

    glm::vec2 &value = attr.value();

    MetaWidget *widget = make_meta_widget_vbox(parent);
    auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

    if (widget_type.empty()) widget_type = "Input";

    if (!label_txt.empty())
      layout->addWidget(new QLabel(QString::fromStdString(label_txt), widget));

    if (widget_type == "None") // --- None
    {
      return nullptr;
    }
    else if (widget_type == "Input") // --- Input
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
    else if (widget_type == "XYCanvas") // --- XYCanvas
    {
      // Per-axis bounds (compat "xy()" preset stash), falling back to the
      // shared min/max when absent so asymmetric domains (e.g. x in [0,1],
      // y in [0,100]) reach the widget.
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

      // Button row
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

      // Fires on every drag step — edit_started + value_changed only.
      QObject::connect(canvas,
                       &XYCanvas::value_changed,
                       widget,
                       [widget, &attr](glm::vec2)
                       {
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });

      // Fires once on mouse release — edit_ended.
      QObject::connect(canvas,
                       &XYCanvas::drag_ended,
                       widget,
                       [widget, &attr](glm::vec2)
                       {
                         Q_EMIT widget->edit_ended();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });

      // Center
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

      // Random
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
    else if (widget_type == "RangeBar") // --- RangeBar
    {
      // Guard value sentinel: {-1, 0} means "disabled / no range".
      const bool initially_active = !(value.x == -1.f && value.y == 0.f);

      // Restore the last meaningful range when toggling back on.
      // Seeded from the current value if it's valid, otherwise full domain.
      glm::vec2 last_active_value = initially_active ? value
                                                     : glm::vec2{min, max};

      auto *bar = new RangeBar(value, min, max, decimals, widget);

      meta::DataProvider range_provider; // empty if none
      if (const auto *mp = attr.metadata().find(meta::keys::ui::data_provider))
        if (const auto
                *dp = mp->try_cast<meta::Attribute<meta::DataProvider>>())
          range_provider = dp->value();

      if (range_provider)
      {
        try
        {
          auto data = range_provider();
          if (auto histogram = data.get<HistogramData>())
            if (!histogram->y.empty())
              bar->set_histogram(histogram->x, histogram->y);
        }
        catch (...)
        {
          // a faulty host provider must not crash the panel
        }
      }

      auto *btn_row = new QHBoxLayout();
      auto *toggle_btn = new QPushButton(widget);
      auto *reset_btn = new QPushButton(QObject::tr("Full"), widget);
      auto *center_btn = new QPushButton(QObject::tr("Center"), widget);
      auto *unit_btn = new QPushButton(QObject::tr("[0, 1]"), widget);

      toggle_btn->setCheckable(true);
      toggle_btn->setChecked(initially_active);
      toggle_btn->setText(initially_active ? QObject::tr("On")
                                           : QObject::tr("Off"));
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

      // Enable or disable all range controls according to the sentinel state.
      auto set_active = [bar, reset_btn, center_btn, unit_btn](bool active)
      {
        bar->setEnabled(active);
        reset_btn->setEnabled(active);
        center_btn->setEnabled(active);
        unit_btn->setEnabled(active);
      };

      set_active(initially_active);

      widget->set_sync_from_model(
          [&value, bar, toggle_btn, set_active, widget, range_provider]()
          {
            const bool active = !(value.x == -1.f && value.y == 0.f);

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
                  if (!histogram->y.empty())
                    bar->set_histogram(histogram->x, histogram->y);
              }
              catch (...)
              {
              }
            }
          });

      // Toggle
      QObject::connect(toggle_btn,
                       &QPushButton::toggled,
                       widget,
                       [&value,
                        &attr,
                        bar,
                        toggle_btn,
                        set_active,
                        widget,
                        // Mutable copy so each lambda invocation can update it.
                        lav = last_active_value](bool active) mutable
                       {
                         toggle_btn->setText(active ? QObject::tr("On")
                                                    : QObject::tr("Off"));
                         set_active(active);

                         if (active)
                         {
                           // Restore last known good range.
                           attr.set_from_any(lav);
                           bar->set_value(lav);
                         }
                         else
                         {
                           // Save current range before clobbering it.
                           lav = value;
                           attr.set_from_any(glm::vec2{-1.f, 0.f});
                           bar->set_value({-1.f, 0.f});
                         }

                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       });

      // Live drag → edit_started + value_changed
      QObject::connect(bar,
                       &RangeBar::value_changed,
                       widget,
                       [widget](glm::vec2)
                       {
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                       });

      // Release → edit_ended
      QObject::connect(bar,
                       &RangeBar::drag_ended,
                       widget,
                       [widget](glm::vec2) { Q_EMIT widget->edit_ended(); });

      // Full domain
      QObject::connect(reset_btn,
                       &QPushButton::clicked,
                       widget,
                       [&attr, min, max, bar, widget]()
                       {
                         bar->set_value({min, max});
                         attr.set_from_any(glm::vec2{min, max});
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       });

      // Center — shift span to domain midpoint
      QObject::connect(
          center_btn,
          &QPushButton::clicked,
          widget,
          [&value, &attr, min, max, bar, widget]()
          {
            const float span = value.y - value.x;
            const float mid = 0.f; // (min + max) * 0.5f;
            const float lo = std::clamp(mid - span * 0.5f, min, max - span);
            bar->set_value({lo, lo + span});
            attr.set_from_any(glm::vec2{lo, lo + span});
            Q_EMIT widget->edit_started();
            Q_EMIT widget->value_changed();
            Q_EMIT widget->edit_ended();
          });

      // Unit — [0, 1] clamped to domain
      QObject::connect(unit_btn,
                       &QPushButton::clicked,
                       widget,
                       [&attr, min, max, bar, widget]()
                       {
                         const float lo = std::clamp(0.f, min, max);
                         const float hi = std::clamp(1.f, min, max);
                         bar->set_value({lo, hi});
                         attr.set_from_any(glm::vec2{lo, hi});
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       });
    }
    else if (widget_type == "VectorEditor") // --- VectorEditor
    {
      // --- Canvas

      auto *canvas = new VectorCanvas(value, max, locked_xy, widget);

      // Centre the fixed-size canvas horizontally.
      auto *canvas_row = new QHBoxLayout();
      canvas_row->addStretch();
      canvas_row->addWidget(canvas);
      canvas_row->addStretch();
      layout->addLayout(canvas_row);

      // --- Controls

      auto *form = new QFormLayout();
      form->setContentsMargins(0, 2, 0, 2);
      form->setSpacing(3);

      // Magnitude spinbox
      auto *mag_spin = new QDoubleSpinBox(widget);
      mag_spin->setRange(0.0, double(max));
      mag_spin->setDecimals(decimals);
      mag_spin->setSingleStep(double(max) / 100.0);
      mag_spin->setValue(double(canvas->magnitude()));
      mag_spin->setFixedHeight(22);

      // Angle spinbox (disabled when locked)
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

      // Lock toggle
      auto *lock_row = new QHBoxLayout();
      auto *lock_cb = new QCheckBox(QObject::tr("Isotropic  (kx = ky)"),
                                    widget);
      lock_cb->setChecked(locked_xy);
      lock_row->addStretch();
      lock_row->addWidget(lock_cb);
      layout->addLayout(lock_row);

      // --- Sync helpers

      widget->set_sync_from_model(
          [state, widget_type, &value, canvas, mag_spin, angle_spin, lock_cb]()
          {
            float mag = glm::length(value);
            float deg = (mag > 1e-6f)
                            ? std::atan2(value.y, value.x) * 180.f / float(M_PI)
                            : 45.f;
            bool  stored_locked_state = state->metadata().value<bool>(
                widget_type + ".locked_xy");

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

      // Canvas → spinboxes (keep in sync after drag)
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

      // Magnitude spinbox → canvas
      QObject::connect(mag_spin,
                       qOverload<double>(&QDoubleSpinBox::valueChanged),
                       widget,
                       [canvas](double v) { canvas->set_magnitude(float(v)); });

      // Angle spinbox → canvas
      QObject::connect(angle_spin,
                       qOverload<double>(&QDoubleSpinBox::valueChanged),
                       widget,
                       [canvas](double v) { canvas->set_angle_deg(float(v)); });

      // Lock toggle → canvas + angle spinbox enable state
      QObject::connect(
          lock_cb,
          &QCheckBox::toggled,
          widget,
          [&attr, state, widget_type, canvas, angle_spin](bool checked)
          {
            canvas->set_locked(checked);
            angle_spin->setEnabled(!checked);

            state->metadata().value<bool>(widget_type + ".locked_xy") = checked;

            // not using 'set_from_any' method, force emit
            attr.value_changed.notify(attr.value());
          });

      // --- Graph signals

      // Live drag / spinbox change
      QObject::connect(canvas,
                       &VectorCanvas::value_changed,
                       widget,
                       [&attr, widget](glm::vec2)
                       {
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });

      // Committed (drag release, lock toggle, spinbox enter)
      QObject::connect(canvas,
                       &VectorCanvas::drag_ended,
                       widget,
                       [&attr, widget](glm::vec2)
                       {
                         Q_EMIT widget->edit_ended();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });

      // Spinboxes commit on editingFinished (Return / focus-out)
      auto spinbox_commit = [&attr, widget]()
      {
        Q_EMIT widget->edit_ended();

        // not using 'set_from_any' method, force emit
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

      // Lock toggle commits immediately
      QObject::connect(lock_cb,
                       &QCheckBox::toggled,
                       widget,
                       [&attr, widget](bool)
                       {
                         Q_EMIT widget->edit_ended();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });
    }
    else if (widget_type == "LinkedSliders") // --- LinkedSliders
    {
      // Row: [slider X] [slider Y] [X=Y toggle]
      auto *row = new QHBoxLayout();
      row->setSpacing(4);

      auto *slider_x = new SliderFloat(x_label,
                                       value.x,
                                       min,
                                       max,
                                       /* plus_minus */ false,
                                       format,
                                       widget);
      auto *slider_y = new SliderFloat(y_label,
                                       value.y,
                                       min,
                                       max,
                                       /* plus_minus */ false,
                                       format,
                                       widget);

      slider_x->set_value(value.x);
      slider_y->set_value(value.y);

      // Lock toggle — compact, fixed width so sliders get most of the space
      auto *lock_btn = new QPushButton(QObject::tr("X=Y"), widget);
      lock_btn->setCheckable(true);
      lock_btn->setChecked(locked_xy);
      lock_btn->setFixedWidth(36);
      lock_btn->setFixedHeight(slider_x->sizeHint().height());
      lock_btn->setToolTip(QObject::tr("Lock X = Y"));

      // Visual feedback: bold/highlighted when locked
      auto update_lock_style = [lock_btn](bool locked)
      {
        lock_btn->setProperty("locked", locked);
        // Simple style: invert background when active
        lock_btn->setStyleSheet(locked ? "font-weight: bold;"
                                       : "font-weight: normal;");
      };
      update_lock_style(locked_xy);

      // Sync Y → X when locked on startup
      if (locked_xy)
      {
        value.y = value.x;
        slider_y->set_value(value.x);
        slider_y->setEnabled(false);
      }

      // The slider pair lives in a ResponsiveBox: side-by-side when there is
      // room, stacked vertically when the panel is too narrow to fit both.
      // The box reports a one-slider minimum width, so the panel is free to
      // narrow below the two-slider width (which is what triggers stacking).
      auto *pair = new ResponsiveBox(widget);
      pair->set_spacing(4);
      pair->add_widget(slider_x, 1); // stretch factor 1
      pair->add_widget(slider_y, 1);

      row->addWidget(pair, 1);
      row->addWidget(lock_btn, 0);
      layout->addLayout(row);

      // --- Connections

      widget->set_sync_from_model(
          [state, widget_type, &value, slider_x, slider_y, lock_btn]()
          {
            {
              QSignalBlocker b(slider_x);
              slider_x->set_value(value.x);
            }

            {
              QSignalBlocker b(slider_y);
              slider_y->set_value(value.y);
            }

            {
              QSignalBlocker b(lock_btn);
              lock_btn->setChecked(
                  state->metadata().value<bool>(widget_type + ".locked_xy"));
            }

            slider_y->setEnabled(!lock_btn->isChecked());
          });

      // slider_x changed
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

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });

      QObject::connect(slider_x,
                       &SliderFloat::edit_ended,
                       widget,
                       [&attr, &value, slider_x, widget]()
                       {
                         value.x = slider_x->get_value();
                         Q_EMIT widget->edit_ended();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });

      // slider_y changed (only reachable when unlocked)
      QObject::connect(slider_y,
                       &SliderFloat::value_changed,
                       widget,
                       [&attr, &value, slider_y, widget]()
                       {
                         value.y = slider_y->get_value();
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });

      QObject::connect(slider_y,
                       &SliderFloat::edit_ended,
                       widget,
                       [&attr, &value, slider_y, widget]()
                       {
                         value.y = slider_y->get_value();
                         Q_EMIT widget->edit_ended();

                         // not using 'set_from_any' method, force emit
                         attr.value_changed.notify(attr.value());
                       });

      // Lock toggle
      QObject::connect(lock_btn,
                       &QPushButton::toggled,
                       widget,
                       [state,
                        widget_type,
                        &value,
                        &attr,
                        slider_x,
                        slider_y,
                        lock_btn,
                        update_lock_style,
                        widget](bool locked)
                       {
                         update_lock_style(locked);
                         slider_y->setEnabled(!locked);

                         if (locked)
                         {
                           // Snap Y to current X immediately
                           value.y = value.x;
                           {
                             QSignalBlocker b(slider_y);
                             slider_y->set_value(value.x);
                           }
                         }

                         state->metadata().value<bool>(widget_type +
                                                       ".locked_xy") = locked;

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
        [widget](glm::vec2) { widget->sync_widget_from_model(); });

    return widget;
  }
};

} // namespace meta::qt
