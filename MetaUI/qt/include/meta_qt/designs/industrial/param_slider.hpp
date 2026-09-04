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

  /** @brief Decline attributes with no usable range.
   *
   * A rail needs `max > min` to span. Without both keys meta::common::min/max
   * return the numeric limits, which produces a rail no drag can meaningfully
   * address -- so the row falls back to the stock spin box instead.
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

  float       min_ = 0.f;
  float       max_ = 1.f;
  float       value_ = 0.f;
  bool        log_scale_ = false;
  int         decimals_ = 2;
  std::string label_;
  std::string category_;
  std::string key_; ///< attribute name, for the defaults lookup on reset

  Glide     *glide_ = nullptr; ///< animates the displayed position, 0..1
  qreal      norm_ = 0.0;      ///< what is painted; may lag value_ mid-glide
  QLineEdit *field_ = nullptr;
  bool       dragging_ = false;
  bool       hovered_rail_ = false;
};

} // namespace meta::qt::industrial
