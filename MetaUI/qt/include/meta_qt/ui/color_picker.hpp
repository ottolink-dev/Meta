/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <functional>

#include <QColor>
#include <QString>

class QWidget;

namespace meta::qt
{

/// Signature of an application-provided colour picker. Returns an invalid
/// QColor when the user cancels.
using ColorPickerFn = std::function<
    QColor(const QColor &initial, QWidget *parent, const QString &title, bool alpha)>;

/// Route every colour pick made by MetaUI widgets through @p fn, so the host
/// application can show a picker in its own style. An empty function restores
/// the default (QColorDialog).
void set_color_picker(ColorPickerFn fn);

/// Ask the user for a colour: the registered picker, or QColorDialog.
QColor pick_color(const QColor  &initial,
                  QWidget       *parent,
                  const QString &title = QString(),
                  bool           alpha = false);

} // namespace meta::qt
