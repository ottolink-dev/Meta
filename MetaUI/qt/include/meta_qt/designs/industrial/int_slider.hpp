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

/** @brief Label / rail / value-field row for an int attribute.
 *
 * SliderInt, roughly 13% of the rows in a Hesiod node panel once Seed is
 * counted.
 *
 * Shares its chrome with ParamSlider through slider_chrome, so the two rows
 * cannot drift apart visually. What differs is the value semantics: the rail
 * quantises to whole numbers, and the glide runs over the integer range rather
 * than a normalised float, so the readout never shows a value the model does
 * not hold.
 */
class IntSlider : public Control<int>
{
  Q_OBJECT

public:
  IntSlider(Attribute<int> &attr, const RowContext &ctx, QWidget *parent = nullptr);

  /// A rail needs max > min to span; without it the row falls back to stock.
  static bool can_render(const Attribute<int> &attr);

  int  get() const override { return value_; }
  void set(const int &value) override;

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
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  qreal to_norm(int value) const;
  int   from_norm(qreal t) const;

  void set_from_position(int x);
  void apply_value(int value, bool glide);
  void refresh_field();
  void restyle_field(bool editing = false);

  int         min_ = 0;
  int         max_ = 1;
  int         value_ = 0;
  std::string label_;
  std::string category_;
  std::string key_;

  Glide     *glide_ = nullptr; ///< animates the painted position only
  qreal      norm_ = 0.0;
  QLineEdit *field_ = nullptr;
  bool       dragging_ = false;
};

} // namespace meta::qt::industrial
