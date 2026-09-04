/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <QLabel>
#include <QLayout>
#include <QWidget>

#include "meta/type/type_name.hpp"
#include "meta_common.hpp"

#include "meta_qt/meta_widget.hpp"

namespace meta::qt
{

/// Render a runtime-typed attribute into a MetaWidget using the default design.
MetaWidget *render(AbstractAttribute *p_attr, QWidget *parent = nullptr);

/// Helper: render a typed attribute into a MetaWidget.
template <typename T>
MetaWidget *render(Attribute<T> &attr, QWidget *parent = nullptr)
{
  return render(&attr, parent);
}

/// Compatibility wrapper forwarding to meta::qt::render().
template <typename T> struct WidgetRenderer
{
  static MetaWidget *render(Attribute<T> &attr, QWidget *parent = nullptr)
  {
    return meta::qt::render(&attr, parent);
  }
};

} // namespace meta::qt