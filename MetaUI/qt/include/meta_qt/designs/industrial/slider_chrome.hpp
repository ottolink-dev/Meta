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

  /** @brief The range has no usable limits, so the thumb is a rate handle.
   *
   * Changes what the rail means. A bounded rail fills from its left edge to
   * the thumb, because the thumb's position *is* the value. An unbounded one
   * has no such position to fill towards: the thumb rests at the centre and
   * reports drag distance, so a fill would be claiming a proportion that does
   * not exist.
   */
  bool unbounded = false;

  /// Drag in progress. Only read when `unbounded`, to mark the rest position.
  bool dragging = false;
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
 * by astronomical steps.
 *
 * This used to gate can_render(), so those rows fell through to stock. That
 * fixed the drag but cost more than it bought: it hit 197 rows, 86 of them the
 * Seed that sits near the top of nearly every node, so almost every panel grew
 * a stock row among the industrial ones. The sliders now carry their own
 * unbounded mode instead, and this selects between the two.
 *
 * The sentinel test deliberately matches stock's `is_range_bounded()` exactly,
 * against the type's own limits. A looser threshold would leave a gap where a
 * merely huge range counted as unbounded here but bounded there, so the two
 * designs would disagree about which control a row gets.
 *
 * Note a half-open range still lands here: Seed is [0, INT_MAX], and a lower
 * bound alone cannot give the rail a span. The real bound is still enforced,
 * it just clamps the value rather than positioning the thumb.
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

/**
 * The value field's font, a pixel smaller at a time until @p text fits in
 * @p field_width. Long readouts (engineering notation, many decimals) then
 * stay whole instead of being cut off by the fixed-width field.
 */
QFont fitted_field_font(const QString &text, int field_width);

} // namespace meta::qt::industrial
