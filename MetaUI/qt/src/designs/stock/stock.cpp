/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/stock/stock.hpp"

#include "meta_qt/ui/design_registry.hpp"
#include "meta_qt/widget_renderer.hpp"

namespace meta::qt::stock
{

namespace
{

/** @brief Wrap WidgetRenderer<T> as a row factory.
 *
 * Registered under the wildcard widget_type: WidgetRenderer<T> already
 * branches on widget_type internally, so splitting those branches into
 * separate entries would be a rewrite rather than a registration. Doing that
 * later would remove the inner if-chains too, but it is not needed to make the
 * registry the single dispatch point.
 */
template <class T> RowFactory wrap()
{
  return [](AbstractAttribute &attr, const RowContext &, QWidget *parent) -> MetaWidget *
  { return WidgetRenderer<T>::render(static_cast<Attribute<T> &>(attr), parent); };
}

template <class T> void add(DesignRegistry &registry)
{
  registry.add(kDesignName, std::type_index(typeid(T)), kAnyWidgetType, wrap<T>());
}

} // namespace

void register_design()
{
  static bool registered = false;
  if (registered) return;
  registered = true;

  DesignRegistry &registry = DesignRegistry::instance();

  add<bool>(registry);
  add<float>(registry);
  add<int>(registry);
  add<std::string>(registry);
  add<std::filesystem::path>(registry);
  add<std::vector<float>>(registry);

#ifdef META_ENABLE_GLM_TYPES
  add<glm::ivec2>(registry);
  add<glm::vec2>(registry);
  add<glm::vec3>(registry);
  add<glm::vec4>(registry);
  add<std::vector<glm::vec3>>(registry);
#endif

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
  add<meta::ColorGradient>(registry);
#endif

#ifdef META_ENABLE_ARRAY_TYPES
  add<meta::Array>(registry);
#endif
}

} // namespace meta::qt::stock
