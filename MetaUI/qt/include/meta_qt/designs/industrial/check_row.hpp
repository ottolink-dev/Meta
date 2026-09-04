/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <string>

#include "meta_common.hpp"

#include "meta_qt/ui/control.hpp"
#include "meta_qt/ui/glide.hpp"

namespace meta::qt::industrial
{

/** @brief Label plus sliding switch for a bool attribute.
 *
 * The industrial design's Toggle, second only to the float slider by volume.
 *
 * Deliberately built on the same Control/bind machinery as ParamSlider despite
 * having no drag and no range -- if the binder needed special-casing for a
 * control this simple, the abstraction would not be carrying its weight.
 */
class CheckRow : public Control<bool>
{
  Q_OBJECT

public:
  CheckRow(Attribute<bool>  &attr,
           const RowContext &ctx,
           QWidget          *parent = nullptr);

  /// A bool always has a renderable state; nothing to decline.
  static bool can_render(const Attribute<bool> &) { return true; }

  bool get() const override { return value_; }
  void set(const bool &value) override;

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private:
  QRect switch_rect() const;
  void  toggle();

  bool        value_ = false;
  std::string label_;
  std::string key_;

  Glide *glide_ = nullptr; ///< knob travel, 0 = off, 1 = on
  qreal  knob_ = 0.0;
  bool   pressed_ = false;
};

} // namespace meta::qt::industrial
