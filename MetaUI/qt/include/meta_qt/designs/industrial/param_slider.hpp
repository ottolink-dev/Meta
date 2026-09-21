/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <string>

#include "meta_common.hpp"

#include "meta_qt/designs/industrial/slider_chrome.hpp"
#include "meta_qt/ui/control.hpp"
#include "meta_qt/ui/glide.hpp"

class QLineEdit;

namespace meta::qt::industrial
{

/** @brief Label / rail / value-field row for a float attribute.
 *
 * The industrial design's SliderFloat, covering roughly 58% of the rows in a
 * Hesiod node panel.
 *
 * Painting follows the state matrix strictly: the rail fill is *always* the
 * group accent and never encodes state; only the label and value text change
 * colour between default, modified and locked.
 */
class ParamSlider : public Control<float>
{
  Q_OBJECT

public:
  ParamSlider(Attribute<float> &attr,
              const RowContext &ctx,
              QWidget          *parent = nullptr);

  /** @brief Accept any attribute that declares a range, bounded or not.
   *
   * Both constraint keys must be present: without them the attribute is not
   * asking to be a slider at all, and meta::common::min/max would invent
   * limits it never declared.
   *
   * The values themselves are not screened. A range with no usable span is
   * rendered in the unbounded mode below rather than declined.
   */
  static bool can_render(const Attribute<float> &attr);

  float get() const override { return value_; }
  void  set(const float &value) override;

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void handle_wheel(QWheelEvent *event) override;
  void on_state_changed() override;

  /// Watches the value field's focus so its chrome can follow the edit state.
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  // --- value <-> normalised position
  qreal to_norm(float value) const;
  float from_norm(qreal t) const;

  void    set_from_position(int x);
  void    apply_norm(qreal t);
  QString format_value(float value) const;
  void    refresh_field();
  void    restyle_field(bool editing = false);

  /** @brief Advance an unbounded drag to cursor position `x`.
   *
   * Moves the value by the distance dragged rather than to the position under
   * the cursor, and moves the thumb to match until it reaches the end of the
   * rail. Ctrl is fine, Shift is coarse, as in stock.
   */
  void drag_by(int x, Qt::KeyboardModifiers modifiers);

  /// Seat `value` and publish it. No animation: use for unbounded rows.
  void apply_value(float value);

  /** @brief Move to `value` as one complete edit.
   *
   * Glides when the rail can show the motion and seats immediately when it
   * cannot, so the three callers that just want "go there and commit" -- typed
   * value, wheel notch, double-click reset -- do not each repeat the branch.
   */
  void commit_value(float value);

  float min_ = 0.f;
  float max_ = 1.f;
  float input_max_ = 1.f;
  float value_ = 0.f;
  bool  log_scale_ = false;
  int   decimals_ = 2;

  /** @brief The attribute's declared format spec, e.g. "{:.2e}".
   *
   * Kept whole rather than reduced to a decimal count, because the
   * presentation type carries as much intent as the precision does. A rate
   * declared over five decades wants 1.00e-03, not 0.001, and certainly not
   * the eight characters of 0.00000100 that a fixed readout produces.
   */
  std::string format_;
  std::string label_;
  std::string category_;
  std::string key_; ///< attribute name, for the defaults lookup on reset

  /** @brief No usable range, so the thumb reports drag rate, not position.
   *
   * Fixed at construction: an attribute's constraints do not change under it.
   */
  bool unbounded_ = false;

  Glide     *glide_ = nullptr; ///< animates the displayed position, 0..1
  qreal      norm_ = 0.0;      ///< what is painted; may lag value_ mid-glide
  QLineEdit *field_ = nullptr;
  bool       dragging_ = false;
  bool       hovered_rail_ = false;

  // --- unbounded drag reference, both only meaningful while dragging_
  int   drag_origin_x_ = 0;
  float value_at_press_ = 0.f;
};

} // namespace meta::qt::industrial
