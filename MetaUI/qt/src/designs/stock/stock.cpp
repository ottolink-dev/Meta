#include "meta_qt/designs/stock/stock.hpp"

#include "meta_qt/designs/stock/stock_renderer.hpp"
#include "meta_qt/ui/design_registry.hpp"
#include "meta_qt/widgets/collapsible_section.hpp"

// Stock widget builders
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

namespace meta::qt::stock
{

namespace
{

/** @brief Wrap StockRenderer<T> as a row factory.
 *
 * Registered under specific widget_types and the wildcard widget_type.
 */
template <class T> RowFactory wrap()
{
  return [](AbstractAttribute &attr,
            const RowContext &,
            QWidget *parent) -> MetaWidget * {
    return StockRenderer<T>::render(static_cast<Attribute<T> &>(attr), parent);
  };
}

template <class T>
void add(DesignRegistry    &registry,
         const std::string &widget_type = kAnyWidgetType)
{
  registry.add(kDesignName, std::type_index(typeid(T)), widget_type, wrap<T>());
}

} // namespace

void register_design()
{
  static bool registered = false;
  if (registered) return;
  registered = true;

  DesignRegistry &registry = DesignRegistry::instance();

  // --- Section factory
  registry.register_section_factory(kDesignName,
                                    [](const QString &title)
                                    { return new CollapsibleSection(title); });

  // --- Granular registrations for common widgets
  add<bool>(registry, "Toggle");
  add<bool>(registry, "Checkbox");
  add<bool>(registry, "BinaryButtons");
  add<bool>(registry, kAnyWidgetType);

  add<float>(registry, "Input");
  add<float>(registry, "Slider");
  add<float>(registry, "ScrollBar");
  add<float>(registry, "Dial");
  add<float>(registry, "SliderFloat");
  add<float>(registry, kAnyWidgetType);

  add<int>(registry, "Input");
  add<int>(registry, "Slider");
  add<int>(registry, "ScrollBar");
  add<int>(registry, "Dial");
  add<int>(registry, "SliderInt");
  add<int>(registry, "EnumComboBox");
  add<int>(registry, kAnyWidgetType);

  add<std::string>(registry, "ComboBox");
  add<std::string>(registry, "ButtonGrid");
  add<std::string>(registry, "SingleLineText");
  add<std::string>(registry, "MultilineText");
  add<std::string>(registry, "CodeEditor");
  add<std::string>(registry, "ReadOnlyText");
  add<std::string>(registry, kAnyWidgetType);

  add<std::filesystem::path>(registry, "OpenFile");
  add<std::filesystem::path>(registry, "SaveFile");
  add<std::filesystem::path>(registry, "Directory");
  add<std::filesystem::path>(registry, kAnyWidgetType);

  add<std::vector<float>>(registry, kAnyWidgetType);

#ifdef META_ENABLE_GLM_TYPES
  add<glm::ivec2>(registry, kAnyWidgetType);
  add<glm::vec2>(registry, "XYCanvas");
  add<glm::vec2>(registry, "VectorEditor");
  add<glm::vec2>(registry, "LinkedSliders");
  add<glm::vec2>(registry, "RangeBar");
  add<glm::vec2>(registry, kAnyWidgetType);
  add<glm::vec3>(registry, "ColorPicker");
  add<glm::vec3>(registry, kAnyWidgetType);
  add<glm::vec4>(registry, "ColorPicker");
  add<glm::vec4>(registry, kAnyWidgetType);
  add<std::vector<glm::vec3>>(registry, "PointsEditor");
  add<std::vector<glm::vec3>>(registry, "PathEditor");
  add<std::vector<glm::vec3>>(registry, kAnyWidgetType);
#endif

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
  add<meta::ColorGradient>(registry, kAnyWidgetType);
#endif

#ifdef META_ENABLE_ARRAY_TYPES
  add<meta::Array>(registry, kAnyWidgetType);
#endif
}

} // namespace meta::qt::stock
