/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <cmath>

#include <QSignalBlocker>

#include "meta_common.hpp"

#include "meta_qt/meta_widget.hpp"
#include "meta_qt/ui/control.hpp"

namespace meta::qt
{

/** @brief Equality used to decide whether a value differs from its default.
 *
 * Specialise for types where exact comparison is wrong. Floating point uses the
 * 1e-4 tolerance the state matrix is defined in terms of.
 */
template <class T> struct ValueCompare
{
  static bool equal(const T &a, const T &b) { return a == b; }
};

template <> struct ValueCompare<float>
{
  static bool equal(float a, float b) { return std::fabs(a - b) <= 1e-4f; }
};

template <> struct ValueCompare<double>
{
  static bool equal(double a, double b) { return std::fabs(a - b) <= 1e-4; }
};

/** @brief Wire an attribute to a control, both directions.
 *
 * Named bind_control rather than bind: an unqualified bind<T>() call with a std
 * type such as std::string pulls std::bind in through ADL and fails somewhere
 * deep inside <functional>, nowhere near the actual mistake.
 *
 * Written once per *type* and never per design. Every visual variant of a float
 * control shares this function, so the races below are fixed in one place:
 *
 * - A model sync arriving mid-drag re-seats the value under the cursor and
 *   drops the handle. Suppressed via Control::is_editing().
 * - Writing the attribute notifies subscribers synchronously, which syncs back
 *   into the control, which would re-emit. Broken with QSignalBlocker.
 * - The subscription outliving the widget dereferences freed memory. It is
 *   stored in MetaWidget::connection_, declared so it dies first.
 *
 * The control owns range clamping and metadata reading; this function knows
 * neither, which is what keeps it type-generic.
 *
 * Note this does *not* decide when the host recomputes. It emits
 * edit_started/value_changed/edit_ended on `host` and the host chooses which to
 * act on -- that is where a live-update setting belongs.
 */
template <class T>
void bind_control(Attribute<T> &attr, Control<T> &control, MetaWidget &host)
{
  const std::string key = attr.name();

  // --- modified state, recomputed on every value change
  auto refresh_modified = [&attr, &control, key]()
  {
    const auto &provider = control.context().default_value;
    if (!provider)
    {
      control.set_modified(false);
      return;
    }

    const std::any def = provider(key);
    if (!def.has_value())
    {
      control.set_modified(false);
      return;
    }

    try
    {
      control.set_modified(
          !ValueCompare<T>::equal(attr.value(), std::any_cast<T>(def)));
    }
    catch (const std::bad_any_cast &)
    {
      control.set_modified(false);
    }
  };

  // --- control -> model
  QObject::connect(&control,
                   &ControlBase::edit_started,
                   &host,
                   [&host]() { Q_EMIT host.edit_started(); });

  QObject::connect(&control,
                   &ControlBase::value_changed,
                   &host,
                   [&attr, &control, &host, refresh_modified]()
                   {
                     attr.set_from_any(control.get());
                     refresh_modified();
                     Q_EMIT host.value_changed();
                   });

  QObject::connect(&control,
                   &ControlBase::edit_ended,
                   &host,
                   [&attr, &control, &host, refresh_modified]()
                   {
                     attr.set_from_any(control.get());
                     refresh_modified();
                     Q_EMIT host.edit_ended();
                   });

  // --- model -> control
  host.set_sync_from_model(
      [&attr, &control, refresh_modified]()
      {
        // A sync must never fight an edit in progress.
        if (control.is_editing()) return;

        const QSignalBlocker blocker(&control);
        control.set(attr.value());
        refresh_modified();
      });

  // Dies with the widget: connection_ is declared before anything it captures.
  host.connection_ = attr.value_changed.subscribe(
      [&host](const T &) { host.sync_widget_from_model(); });

  // --- initial state
  control.set_locked(
      meta::common::try_get<bool>(attr, meta::keys::ui::read_only, false));
  refresh_modified();
}

} // namespace meta::qt
