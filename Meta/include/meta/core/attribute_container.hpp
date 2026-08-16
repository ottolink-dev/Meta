/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

/**
 * @file attribute_container.hpp
 * @brief Container for named runtime attributes with lookup and JSON support.
 * @copyright Copyright (c) 2026
 */
#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <nlohmann/json.hpp>

#include "meta/core/attribute.hpp"
#include "meta/serialization/snapshot_manager.hpp"

#include <iostream>

namespace meta
{

/**
 * @brief Internal storage type for attributes.
 *
 * Maps attribute names to polymorphic attribute instances.
 */
using AttrContainerType =
    std::unordered_map<std::string, std::unique_ptr<AbstractAttribute>>;

/// Mutable iterator over attributes.
using AttrIterator = AttrContainerType::iterator;

/// Const iterator over attributes.
using ConstAttrIterator = AttrContainerType::const_iterator;

template <typename T>
concept StringLike = std::is_same_v<std::decay_t<T>, std::string> ||
                     std::is_same_v<std::decay_t<T>, const char *> ||
                     std::is_same_v<std::decay_t<T>, char *> ||
                     std::is_same_v<std::decay_t<T>, std::string_view>;

/**
 * @brief Container of named runtime attributes.
 *
 * Provides:
 * - ownership of attributes
 * - fast lookup by name
 * - type-safe access
 * - iteration support
 * - JSON serialization/deserialization
 */
class AttributeContainer
{
public:
  // -------------------------------------------------------------------------
  // Capacity
  // -------------------------------------------------------------------------

  /// Returns true if no attributes are stored.
  bool empty() const noexcept;

  /// Returns number of stored attributes.
  std::size_t size() const noexcept;

  // -------------------------------------------------------------------------
  // Iteration
  // -------------------------------------------------------------------------

  /// Iterator to first attribute.
  AttrIterator begin();

  /// Iterator to end.
  AttrIterator end();

  /// Const iterator to first attribute.
  ConstAttrIterator begin() const;

  /// Const iterator to end.
  ConstAttrIterator end() const;

  /// Const iterator to first attribute (explicit).
  ConstAttrIterator cbegin() const;

  /// Const iterator to end (explicit).
  ConstAttrIterator cend() const;

  // -------------------------------------------------------------------------
  // Lookup
  // -------------------------------------------------------------------------

  /// Returns true if an attribute exists.
  bool contains(const std::string &name) const;

  /// Returns true if all keys exist in the container.
  bool contains_all_keys(const std::vector<std::string> &keys);

  /// Finds an attribute by name.
  AbstractAttribute *find(const std::string &name);

  /// Finds an attribute by name (const).
  const AbstractAttribute *find(const std::string &name) const;

  // -------------------------------------------------------------------------
  // Modification
  // -------------------------------------------------------------------------

  /// Removes all attributes.
  void clear();

  /**
   * @brief Creates and inserts a new attribute.
   *
   * Stores value as Attribute<std::decay_t<T>>.
   *
   * @throws std::runtime_error if the attribute already exists.
   */
  template <typename T>
    requires(!StringLike<T>)
  Attribute<std::decay_t<T>> *add(const std::string &name, T &&value)
  {
    using ValueType = std::decay_t<T>;

    auto attr = std::make_unique<Attribute<ValueType>>(name,
                                                       std::forward<T>(value));

    auto *ptr = attr.get();

    auto [it, inserted] = attributes_.try_emplace(name, std::move(attr));

    if (!inserted)
      throw std::runtime_error("Attribute already exists: " + name);

    // keep track of insertion order
    insertion_order_.push_back(name);

    return ptr;
  }

  /**
   * @brief Creates and inserts an attribute if it does not already exist.
   *
   * If an attribute with the specified name already exists, the existing
   * attribute is returned and no new attribute is created.
   *
   * @tparam T Attribute value type.
   * @param name Name of the attribute.
   * @param value Value used to initialize the attribute if it is created.
   * @return Pointer to the existing or newly created attribute.
   */
  template <typename T>
    requires(!StringLike<T>)
  Attribute<std::decay_t<T>> *try_add(const std::string &name, T &&value)
  {
    using ValueType = std::decay_t<T>;

    // Already exists
    if (auto it = attributes_.find(name); it != attributes_.end())
      return static_cast<Attribute<ValueType> *>(it->second.get());

    auto attr = std::make_unique<Attribute<ValueType>>(name,
                                                       std::forward<T>(value));

    auto *ptr = attr.get();

    attributes_.emplace(name, std::move(attr));

    // keep track of insertion order
    insertion_order_.push_back(name);

    return ptr;
  }

  /// Creates and inserts a std::string attribute.
  Attribute<std::decay_t<std::string>> *add(const std::string &name,
                                            std::string      &&value)
  {
    using ValueType = std::decay_t<std::string>;

    auto attr = std::make_unique<Attribute<ValueType>>(
        name,
        std::forward<std::string>(value));

    auto *ptr = attr.get();

    auto [it, inserted] = attributes_.try_emplace(name, std::move(attr));

    if (!inserted)
      throw std::runtime_error("Attribute already exists: " + name);

    // keep track of insertion order
    insertion_order_.push_back(name);

    return ptr;
  }

  /**
   * @brief Creates and inserts a std::string attribute if it does not already
   * exist.
   *
   * @return Pointer to the existing or newly created attribute.
   */
  Attribute<std::decay_t<std::string>> *try_add(const std::string &name,
                                                std::string      &&value)
  {
    using ValueType = std::decay_t<std::string>;

    // Already exists
    if (auto it = attributes_.find(name); it != attributes_.end())
      return static_cast<Attribute<ValueType> *>(it->second.get());

    auto attr = std::make_unique<Attribute<ValueType>>(
        name,
        std::forward<std::string>(value));

    auto *ptr = attr.get();

    attributes_.emplace(name, std::move(attr));

    // keep track of insertion order
    insertion_order_.push_back(name);

    return ptr;
  }

  // -------------------------------------------------------------------------
  // Accessors
  // -------------------------------------------------------------------------

  /// Returns the attribute names in insertion order.
  const std::vector<std::string> &insertion_order() const;

  /**
   * @brief Reorders the attribute names in insertion order.
   * @param order New ordering of attribute names; must be a permutation of
   *        the current keys.
   * @return true and applies the reorder, or false (leaving order
   *         unchanged) if `order` is not a permutation of the current keys.
   */
  bool set_insertion_order(const std::vector<std::string> &order);

  /**
   * @brief Returns a pointer to the attribute value if it exists and has the
   * requested type.
   * @return Pointer to the value, or nullptr if the attribute is missing or has
   * an incompatible type.
   */
  template <typename T> T *try_value(const std::string &name)
  {
    auto *attr = find(name);

    if (!attr) return nullptr;

    auto *typed = attr->try_cast<Attribute<T>>();
    return typed ? &typed->value() : nullptr;
  }

  /**
   * @brief Returns a pointer to the attribute value if it exists and has the
   * requested type.
   * @return Pointer to the value, or nullptr if the attribute is missing or has
   * an incompatible type.
   */
  template <typename T> const T *try_value(const std::string &name) const
  {
    auto *attr = find(name);

    if (!attr) return nullptr;

    auto *typed = attr->try_cast<Attribute<T>>();
    return typed ? &typed->value() : nullptr;
  }

  /**
   * @brief Returns the attribute value.
   * @throws std::out_of_range if the attribute does not exist.
   * @throws std::runtime_error if the attribute type is incompatible.
   */
  template <typename T> T &value(const std::string &name)
  {
    if (auto *ptr = try_value<T>(name)) return *ptr;

    auto *attr = find(name);

    if (!attr) throw std::out_of_range("Attribute does not exist: " + name);

    throw std::runtime_error("Attribute '" + name + "' has type '" +
                             std::string(attr->type().name()) +
                             "', expected '" + std::string(typeid(T).name()) +
                             "'");
  }

  /**
   * @brief Returns the attribute value.
   * @throws std::out_of_range if the attribute does not exist.
   * @throws std::runtime_error if the attribute type is incompatible.
   */
  template <typename T> const T &value(const std::string &name) const
  {
    if (auto *ptr = try_value<T>(name)) return *ptr;

    auto *attr = find(name);

    if (!attr) throw std::out_of_range("Attribute does not exist: " + name);

    throw std::runtime_error("Attribute '" + name + "' has type '" +
                             std::string(attr->type().name()) +
                             "', expected '" + std::string(typeid(T).name()) +
                             "'");
  }

  // -------------------------------------------------------------------------
  // Serialization
  // -------------------------------------------------------------------------

  /// Serializes container to JSON.
  nlohmann::json json_to(
      SerializationMode mode = SerializationMode::full) const;

  /**
   * @brief Deserializes container from JSON.
   *
   * In Full mode: existing attributes are updated; missing ones are created via
   * factory. In State mode: only declared attributes are updated; undeclared
   * ones are skipped.
   */
  void json_from(const nlohmann::json &j, bool exclude_snapshot_manager)
  {
    json_from(j, SerializationMode::full, exclude_snapshot_manager);
  }

  void json_from(const nlohmann::json &j,
                 SerializationMode     mode = SerializationMode::full,
                 bool                  exclude_snapshot_manager = true);

  /**
   * @brief Access the snapshot manager.
   * @return Reference to the snapshot manager.
   */
  SnapshotManager &snapshot_manager();

  /**
   * @brief Access the snapshot manager.
   * @return Const reference to the snapshot manager.
   */
  const SnapshotManager &snapshot_manager() const;

private:
  AttrContainerType        attributes_;
  std::vector<std::string> insertion_order_;
  SnapshotManager          snapshot_manager_;

  /**
   * @brief Removes stale entries from insertion_order_ that no longer exist
   *        in attributes_ (e.g. after external erasure or clear).
   */
  void compact_insertion_order();
};

// -----------------------------------------------------------------------------
// Metadata / State helpers (external API)
// -----------------------------------------------------------------------------

/// Serialize metadata container to JSON.
nlohmann::json serialize_metadata(const AttributeContainer &m);

/// Deserialize metadata container from JSON.
void deserialize_metadata(AttributeContainer &m, const nlohmann::json &j);

/// Serialize state container to JSON.
nlohmann::json serialize_state(const AttributeContainer &s);

/// Deserialize state container from JSON.
void deserialize_state(AttributeContainer &s, const nlohmann::json &j);

} // namespace meta