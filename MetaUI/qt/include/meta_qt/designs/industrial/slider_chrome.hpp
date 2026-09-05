/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <limits>

#include <QPainterPath>
#include <QRect>
#include <QString>

#include "meta_qt/ui/theme.hpp"

class QPainter;

namespace meta::qt::industrial
{

/** @brief Geometry of one label / rail / value-field row.
 *
 * Shared so the float and int sliders cannot drift apart. Every measurement
 * keys off the row's own width, never the window's.
 */
struct SliderGeometry
{
  QRect label; ///< label column, left
  QRect rail;  ///< recessed well the fill and thumb sit in
  QRect fill;  ///< accent portion of the rail, left of the thumb
  QRect thumb; ///< machined handle
  QRect field; ///< value readout, right

  static SliderGeometry compute(const Theme &theme,
                                int          width,
                                int          height,
                                qreal        norm);
};

/// What the shared painter needs to know about the row's current state.
struct SliderVisual
{
  QString     label;
  std::string category; ///< selects the group accent for the rail fill
  bool        modified = false;
  bool        locked = false;
};

/** @brief Paint label, rail well, accent fill and thumb.
 *
 * The value field is a real QLineEdit owned by the control, so it is not
 * painted here, only measured.
 *
 * Elision is the caller's job because it needs the control's tooltip; pass a
 * label that already fits.
 */
void paint_slider_row(QPainter             &painter,
                      const Theme          &theme,
                      const SliderGeometry &geometry,
                      const SliderVisual   &visual,
                      int                   height);

/// Stylesheet for the value field, following the theme and row state.
/** @brief True when a rail can meaningfully represent the range [lo, hi].
 *
 * A bound of FLT_MAX or INT_MAX does not mean "a very wide slider", it means
 * "no limit". A rail a couple of hundred pixels wide cannot show that: every
 * value a user would type lands in the first pixel, and a drag moves the value
 * by astronomical steps. That is the broken behaviour on the unbounded rows.
 *
 * Declining them here lets them fall through to stock, whose SliderFloat has a
 * proper unbounded mode: the handle sits centred at rest and drags relatively
 * instead of mapping to an absolute position. Using the fallback chain is the
 * point of the design registry, so this belongs in can_render() rather than as
 * a special case inside the paint code.
 *
 * The sentinel test deliberately matches stock's `is_range_bounded()` exactly,
 * against the type's own limits. A looser threshold would leave a gap where a
 * merely huge range is declined here but still counted as bounded there, which
 * breaks it in both designs rather than neither.
 */
template <typename T> bool has_usable_range(T lo, T hi)
{
  // Written as a positive test so a NaN bound falls out here rather than
  // passing an inverted comparison.
  if (!(hi > lo)) return false;

  return lo > std::numeric_limits<T>::lowest() &&
         hi < std::numeric_limits<T>::max();
}

QString field_stylesheet(const Theme &theme,
                         bool         editing,
                         bool         modified,
                         bool         locked);

} // namespace meta::qt::industrial
