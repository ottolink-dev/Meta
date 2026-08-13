/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

/**
 * @file type_registry.hpp
 * @brief Runtime factory and registry for attribute type instantiation.
 * @copyright Copyright (c) 2026
 */
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "meta/core/abstract_attribute.hpp"
#include "meta/core/attribute.hpp"

#define META_REGISTER_ATTRIBUTE_TYPE(...)                                      \
  register_attribute_type<__VA_ARGS__>(#__VA_ARGS__)

namespace meta
{

/**
 * @brief Factory for runtime creation of attributes by type name.
 *
 * This registry enables deserialization and dynamic instantiation
 * of attributes without compile-time knowledge of their concrete types.
 *
 * It maps string identifiers to factory functions producing
 * AbstractAttribute instances.
 */
class AttributeFactory
{
public:
  /**
   * @brief Function type used to create attributes.
   *
   * @param name Attribute name.
   * @return Newly created attribute instance.
   */
  using CreateFn =
      std::function<std::unique_ptr<AbstractAttribute>(const std::string &)>;

  /// Access singleton instance.
  static AttributeFactory &instance()
  {
    static AttributeFactory r;
    return r;
  }

  /**
   * @brief Register a new attribute type.
   *
   * Associates a type name with a factory function.
   *
   * @param name Type identifier used in serialization.
   * @param fn Factory function creating the attribute.
   */
  void register_attribute_type(std::string name, CreateFn fn)
  {
    registry_[std::move(name)] = std::move(fn);
  }

  /**
   * @brief Create an attribute from a type name.
   *
   * @param type Registered type identifier.
   * @param name Attribute instance name.
   * @return Newly created attribute or nullptr if type is unknown.
   */
  std::unique_ptr<AbstractAttribute> create(const std::string &type,
                                            const std::string &name) const;

private:
  std::unordered_map<std::string, CreateFn> registry_;
};

// -----------------------------------------------------------------------------
// Helper API
// -----------------------------------------------------------------------------

/**
 * @brief Register a concrete attribute type in the factory.
 *
 * @tparam T Attribute value type.
 * @param name Type identifier used in JSON serialization.
 */
template <typename T> void register_attribute_type(const std::string &name)
{
  auto &r = AttributeFactory::instance();

  r.register_attribute_type(
      name,
      [](const std::string &attr_name)
      { return std::make_unique<Attribute<T>>(attr_name, T{}); });
}

/**
 * @brief Register all built-in attribute types.
 *
 * Invoked automatically on first use of AttributeFactory::create(); hosts do
 * not need to call it. Calling it manually remains safe.
 */
void register_builtin_types();

} // namespace meta