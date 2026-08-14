/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <mutex>

#include "meta/serialization/attribute_factory.hpp"
#include "meta/core/abstract_attribute.hpp"
#include "meta/core/attribute_container.hpp"
#include "meta/logger.hpp"

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
#include "meta/ext/color_gradient/color_gradient.hpp"
#endif

#ifdef META_ENABLE_ARRAY_TYPES
#include "meta/ext/array/array.hpp"
#endif

namespace meta
{

std::unique_ptr<AbstractAttribute> AttributeFactory::create(
    const std::string &name,
    const std::string &attr_name) const
{
  // Hosts cannot be relied upon to call register_builtin_types() at startup;
  // populate the registry before the first lookup. This lives here rather
  // than in instance() because register_builtin_types() itself calls
  // instance() and would recurse.
  static std::once_flag builtin_types_once;
  std::call_once(builtin_types_once, register_builtin_types);

  Logger::log()->trace("AttributeFactory::create: type='{}', name='{}'",
                       name,
                       attr_name);

  auto it = registry_.find(name);
  if (it == registry_.end())
  {
    Logger::log()->trace("AttributeFactory::create: unknown type '{}'", name);
    return nullptr;
  }

  Logger::log()->trace("AttributeFactory::create: success '{}'", name);
  return it->second(attr_name);
}

void register_builtin_types()
{
  Logger::log()->trace("register_builtin_types: start");

  META_REGISTER_ATTRIBUTE_TYPE(int);
  META_REGISTER_ATTRIBUTE_TYPE(float);
  META_REGISTER_ATTRIBUTE_TYPE(double);
  META_REGISTER_ATTRIBUTE_TYPE(bool);

  META_REGISTER_ATTRIBUTE_TYPE(uint8_t);
  META_REGISTER_ATTRIBUTE_TYPE(uint16_t);
  META_REGISTER_ATTRIBUTE_TYPE(uint32_t);
  META_REGISTER_ATTRIBUTE_TYPE(uint64_t);

  META_REGISTER_ATTRIBUTE_TYPE(std::string);
  META_REGISTER_ATTRIBUTE_TYPE(std::filesystem::path);

  META_REGISTER_ATTRIBUTE_TYPE(std::vector<float>);
  META_REGISTER_ATTRIBUTE_TYPE(std::vector<double>);
  META_REGISTER_ATTRIBUTE_TYPE(std::vector<int>);
  META_REGISTER_ATTRIBUTE_TYPE(std::vector<bool>);

  META_REGISTER_ATTRIBUTE_TYPE(std::vector<std::string>);
  META_REGISTER_ATTRIBUTE_TYPE(std::vector<std::pair<int, std::string>>);

#ifdef META_ENABLE_GLM_TYPES
  Logger::log()->trace("register_builtin_types: GLM enabled");

  META_REGISTER_ATTRIBUTE_TYPE(glm::vec2);
  META_REGISTER_ATTRIBUTE_TYPE(glm::vec3);
  META_REGISTER_ATTRIBUTE_TYPE(glm::vec4);

  META_REGISTER_ATTRIBUTE_TYPE(glm::ivec2);
  META_REGISTER_ATTRIBUTE_TYPE(glm::ivec3);
  META_REGISTER_ATTRIBUTE_TYPE(glm::ivec4);

  META_REGISTER_ATTRIBUTE_TYPE(std::vector<glm::vec2>);
  META_REGISTER_ATTRIBUTE_TYPE(std::vector<glm::vec3>);
  META_REGISTER_ATTRIBUTE_TYPE(std::vector<glm::vec4>);

  META_REGISTER_ATTRIBUTE_TYPE(std::vector<glm::ivec2>);
  META_REGISTER_ATTRIBUTE_TYPE(std::vector<glm::ivec3>);
  META_REGISTER_ATTRIBUTE_TYPE(std::vector<glm::ivec4>);
#endif

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
  Logger::log()->trace("register_builtin_types: ColorGradient enabled");
  META_REGISTER_ATTRIBUTE_TYPE(meta::ColorGradient);
#endif

#ifdef META_ENABLE_ARRAY_TYPES
  Logger::log()->trace("register_builtin_types: Array enabled");
  META_REGISTER_ATTRIBUTE_TYPE(meta::Array);
#endif

  Logger::log()->trace("register_builtin_types: done");
}

} // namespace meta
