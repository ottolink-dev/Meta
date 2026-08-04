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

/// Fallback widget renderer for unsupported types.
template <typename T> struct WidgetRenderer
{
  /// Render an attribute of unsupported type as an error widget.
  static MetaWidget *render(Attribute<T> &attr, QWidget *parent)
  {
    std::string msg;
    msg.reserve(128);
    msg += "Unsupported type: ";
    msg += TypeName<T>::name;
    msg += ", ";
    msg += attr.name();

    MetaWidget *widget = make_meta_widget_hbox(parent);
    auto       *layout = widget->layout();

    QLabel *label = new QLabel(msg.c_str(), widget);
    layout->addWidget(label);

    return widget;
  }
};

/// Helper: render a typed attribute into a MetaWidget.
template <typename T>
MetaWidget *render(Attribute<T> &attr, QWidget *parent = nullptr)
{
  return WidgetRenderer<T>::render(attr, parent);
}

/// Render a runtime-typed attribute into a MetaWidget.
MetaWidget *render(AbstractAttribute *p_attr, QWidget *parent = nullptr);

} // namespace meta::qt

/// /!\ also update widget_renderer.cpp

#include "meta_qt/widget_renderer_inl/bool.inl"
#include "meta_qt/widget_renderer_inl/float.inl"
#include "meta_qt/widget_renderer_inl/int.inl"

#include "meta_qt/widget_renderer_inl/std_filesystem_path.inl"
#include "meta_qt/widget_renderer_inl/std_string.inl"
#include "meta_qt/widget_renderer_inl/std_vector_float.inl"

#ifdef META_ENABLE_GLM_TYPES
#include "meta_qt/widget_renderer_inl/glm_ivec2.inl"
#include "meta_qt/widget_renderer_inl/glm_vec2.inl"
#include "meta_qt/widget_renderer_inl/glm_vec3.inl"
#include "meta_qt/widget_renderer_inl/glm_vec4.inl"

#include "meta_qt/widget_renderer_inl/std_vector_glm_vec3.inl"
#endif

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
#include "meta_qt/widget_renderer_inl/color_gradient.inl"
#endif

#ifdef META_ENABLE_ARRAY_TYPES
#include "meta_qt/widget_renderer_inl/array.inl"
#endif