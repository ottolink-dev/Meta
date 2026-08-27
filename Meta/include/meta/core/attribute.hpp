/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

/**
 * @file attribute.hpp
 * @brief Typed runtime attribute with reflection, serialization, and metadata.
 * @copyright Copyright (c) 2026
 */
#pragma once
#include <any>
#include <string>
#include <typeinfo>

#include <nlohmann/json.hpp>

#include "meta/core/abstract_attribute.hpp"
#include "meta/core/event.hpp"
#include "meta/type/attribute_traits.hpp"
#include "meta/type/type_name.hpp"

namespace meta
{

// -----------------------------------------------------------------------------
// Forward declarations (metadata / state coupling avoidance)
// -----------------------------------------------------------------------------

class AttributeContainer;

/// Serialize attribute metadata to JSON.
nlohmann::json serialize_metadata(const AttributeContainer &m);

/// Deserialize attribute metadata from JSON.
void deserialize_metadata(AttributeContainer &m, const nlohmann::json &j);

/// Serialize attribute state to JSON.
nlohmann::json serialize_state(const AttributeContainer &s);

/// Deserialize attribute state from JSON.
void deserialize_state(AttributeContainer &s, const nlohmann::json &j);

// -----------------------------------------------------------------------------
// Attribute
// -----------------------------------------------------------------------------

/**
 * @brief Typed runtime attribute with reflection support.
 *
 * Stores a strongly typed value with runtime introspection, conversion,
 * and optional metadata.
 *
 * @tparam T Stored value type.
 */
template <typename T> class Attribute : public AbstractAttribute
{
public:
  Event<T> value_changed;

  /**
   * @brief Construct an attribute with a name and initial value.
   *
   * @param name Attribute identifier.
   * @param value Initial stored value.
   */
  Attribute(std::string name, T value)
      : name_(std::move(name)), value_(std::move(value))
  {
    value_changed_conn_ = value_changed.subscribe(
        [this](const T &) { value_changed_event.notify(*this); });
  }

  /// Sets value and notifies subscribers.
  void set_value(const T &new_value)
  {
    value_ = new_value;
    value_changed.notify(value_);
  }

  /// Sets value (by move) and notifies subscribers.
  void set_value(T &&new_value)
  {
    value_ = std::move(new_value);
    value_changed.notify(value_);
  }

  /// Get attribute name.
  const std::string &name() const override { return name_; }

  /// Get runtime type information of stored value.
  std::type_index type() const override { return typeid(T); }

  /// Get mutable pointer to stored value.
  void *raw_ptr() override { return &value_; }

  /// Get const pointer to stored value.
  const void *raw_ptr() const override { return &value_; }

  /**
   * @brief Assign value from type-erased container.
   *
   * @param value Input value wrapped in std::any.
   * @return true if type matches and assignment succeeded.
   */
  bool set_from_any(const std::any &value) override
  {
    if (value.type() != typeid(T)) return false;

    value_ = std::any_cast<T>(value);
    value_changed.notify(value_);
    return true;
  }

  /// Convert value to std::any.
  std::any to_any() const override { return value_; }

  /// Access mutable value.
  T &value() { return value_; }

  /// Access const value.
  const T &value() const { return value_; }

  /// Convert value to human-readable string.
  std::string to_string() const override
  {
    return AttributeTraits<T>::to_string(value_);
  }

  /**
   * @brief Serialize attribute to JSON.
   *
   * In Full mode:
   * - type identifier
   * - value serialization
   * - metadata container
   * - state container
   *
   * In State mode:
   * - value serialization
   * - state container
   */
  nlohmann::json json_to(
      SerializationMode mode = SerializationMode::full) const override
  {
    if (mode == SerializationMode::state)
    {
      nlohmann::json j = {{"value", AttributeTraits<T>::json_to(value_)}};
      nlohmann::json state_json = serialize_state(state());
      if (!state_json.empty())
      {
        j["state"] = state_json;
      }
      return j;
    }

    nlohmann::json j = {{"type", TypeName<T>::name},
                        {"value", AttributeTraits<T>::json_to(value_)}};
    nlohmann::json meta_json = serialize_metadata(metadata());
    if (!meta_json.empty())
    {
      j["metadata"] = meta_json;
    }
    nlohmann::json state_json = serialize_state(state());
    if (!state_json.empty())
    {
      j["state"] = state_json;
    }
    return j;
  }

  /**
   * @brief Deserialize attribute from JSON.
   *
   * Restores:
   * - value
   * - metadata container (only in Full mode)
   * - state container (reconstructed via factory)
   */
  void json_from(const nlohmann::json &j,
                 SerializationMode     mode = SerializationMode::full) override
  {
    if (j.contains("value"))
    {
      value_ = AttributeTraits<T>::json_from(j.at("value"));
      value_changed.notify(value_);
    }
    if (mode == SerializationMode::full && j.contains("metadata"))
    {
      deserialize_metadata(metadata(), j.at("metadata"));
    }
    if (j.contains("state"))
    {
      deserialize_state(state(), j.at("state"));
    }
  }

private:
  std::string     name_;
  T               value_;
  EventConnection value_changed_conn_;
};

} // namespace meta