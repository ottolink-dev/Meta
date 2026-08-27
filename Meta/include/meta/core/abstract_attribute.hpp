/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

/**
 * @file abstract_attribute.hpp
 * @brief Base interface for all reflected attributes.
 * @copyright Copyright (c) 2026
 */
#pragma once
#include <any>
#include <string>
#include <typeindex>

#include <nlohmann/json.hpp>

#include "meta/core/event.hpp"
#include "meta/core/meta_object.hpp"
#include "meta/serialization/serialization_mode.hpp"

namespace meta
{

/**
 * @brief Base class for all attribute types stored in the reflection system.
 *
 * Provides a uniform interface for:
 * - runtime type erasure
 * - generic value access
 * - string and JSON serialization
 * - dynamic modification via std::any
 *
 * All concrete attributes are expected to derive from this class.
 */
class AbstractAttribute : public MetaObject
{
public:
  virtual ~AbstractAttribute() = default;

  /// Untyped event fired whenever the attribute value changes.
  Event<AbstractAttribute &> value_changed_event;

  /// Returns the attribute name.
  virtual const std::string &name() const = 0;

  /// Returns the C++ type identifier of the stored value.
  virtual std::type_index type() const = 0;

  /// Returns a mutable pointer to the underlying value.
  virtual void *raw_ptr() = 0;

  /// Returns a const pointer to the underlying value.
  virtual const void *raw_ptr() const = 0;

  /**
   * @brief Sets the value from a type-erased std::any.
   * @return true if the type matches and assignment succeeded.
   */
  virtual bool set_from_any(const std::any &value) = 0;

  /// Returns the value as a type-erased std::any.
  virtual std::any to_any() const = 0;

  /// Returns a human-readable string representation.
  virtual std::string to_string() const = 0;

  /// Serializes the attribute to JSON.
  virtual nlohmann::json json_to(
      SerializationMode mode = SerializationMode::full) const = 0;

  /// Deserializes the attribute from JSON.
  virtual void json_from(const nlohmann::json &j,
                         SerializationMode mode = SerializationMode::full) = 0;

  /**
   * @brief Attempts to cast this attribute to the specified derived type.
   * @return Pointer to the requested type, or nullptr if the cast fails.
   */
  template <class T = void> T *try_cast()
  {
    T *ptr = dynamic_cast<T *>(this);
    if (ptr)
      return ptr;
    else
      return nullptr;
  }

  /**
   * @brief Attempts to cast this attribute to the specified derived type.
   * @return Pointer to the requested type, or nullptr if the cast fails.
   */
  template <class T = void> const T *try_cast() const
  {
    const T *ptr = dynamic_cast<const T *>(this);
    if (ptr)
      return ptr;
    else
      return nullptr;
  }
};

} // namespace meta