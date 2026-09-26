/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <QColorDialog>

#include "meta_qt/ui/color_picker.hpp"

namespace meta::qt
{

namespace
{
ColorPickerFn &registered_picker()
{
  static ColorPickerFn fn;
  return fn;
}
} // namespace

void set_color_picker(ColorPickerFn fn) { registered_picker() = std::move(fn); }

QColor pick_color(const QColor &initial, QWidget *parent, const QString &title, bool alpha)
{
  if (const ColorPickerFn &fn = registered_picker())
    return fn(initial, parent, title, alpha);

  QColorDialog::ColorDialogOptions options = QColorDialog::DontUseNativeDialog;
  if (alpha)
    options |= QColorDialog::ShowAlphaChannel;
  return QColorDialog::getColor(initial, parent, title, options);
}

} // namespace meta::qt
