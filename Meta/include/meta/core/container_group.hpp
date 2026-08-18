/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

/**
 * @file container_group.hpp
 * @brief Collection of named attribute containers with a selectable active
 * container.
 * @copyright Copyright (c) 2026
 */
#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "meta/core/attribute_container.hpp"
#include "meta/core/meta_object.hpp"

namespace meta
{

/**
 * @brief Collection of named attribute containers.
 *
 * One container can be selected as the current container. Most operations
 * are forwarded to the current container, allowing UI code to interact with
 * a single AttributeContainer interface while the underlying settings group
 * changes at runtime.
 */
class ContainerGroup : public MetaObject
{
public:
  using ContainerMap =
      std::unordered_map<std::string, std::unique_ptr<AttributeContainer>>;

  /**
   * @brief Add a new container.
   * @param key Container identifier.
   * @return Reference to the created container.
   * @throws std::runtime_error if the key already exists.
   */
  AttributeContainer &add(const std::string &key);

  /**
   * @brief Get all registered containers.
   * @return Constant reference to the internal container map.
   */
  const ContainerMap &containers() const;

  /// Check whether a container exists.
  bool contains(const std::string &key) const;

  /// Return the active container.
  AttributeContainer &current();

  /// Return the active container.
  const AttributeContainer &current() const;

  /**
   * @brief Get the name of the active container.
   * @return The current container name if one is selected, otherwise
   * std::nullopt.
   */
  std::optional<std::string> current_container_name() const;

  /**
   * @brief Remove a container.
   * @return true if removed.
   */
  bool erase(const std::string &key);

  /**
   * @brief Find a container by key.
   * @return Pointer to container or nullptr.
   */
  AttributeContainer *find(const std::string &key);

  /**
   * @brief Find a container by key.
   * @return Pointer to container or nullptr.
   */
  const AttributeContainer *find(const std::string &key) const;

  /// Returns the attribute names in insertion order.
  const std::vector<std::string> &insertion_order() const;

  /**
   * @brief Set the active container.
   * @throws std::runtime_error if the container does not exist.
   */
  void set_current(const std::string &key);

  /// Clear all containers.
  void clear();

  // -------------------------------------------------------------------------
  // Serialization
  // -------------------------------------------------------------------------

  /**
   * @brief Serializes the container group to JSON.
   * @param mode Serialization mode (full or state).
   * @return JSON representation of the container group.
   */
  nlohmann::json json_to(
      SerializationMode mode = SerializationMode::full) const;

  /**
   * @brief Deserializes the container group from JSON.
   * @param j JSON object containing the container group data.
   * @param exclude_snapshot_manager Whether to exclude snapshot manager when deserializing individual containers.
   */
  void json_from(const nlohmann::json &j, bool exclude_snapshot_manager)
  {
    json_from(j, SerializationMode::full, exclude_snapshot_manager);
  }

  /**
   * @brief Deserializes the container group from JSON.
   * @param j JSON object containing the container group data.
   * @param mode Serialization mode (full or state).
   * @param exclude_snapshot_manager Whether to exclude snapshot manager when deserializing individual containers.
   */
  void json_from(const nlohmann::json &j,
                 SerializationMode     mode = SerializationMode::full,
                 bool                  exclude_snapshot_manager = true);

  /// Returns the number of groups.
  size_t size() const;

private:
  ContainerMap             containers_;
  std::vector<std::string> insertion_order_;
  AttributeContainer      *current_ = nullptr;

  /**
   * @brief Removes stale entries from insertion_order_ that no longer exist
   *        in attributes_ (e.g. after external erasure or clear).
   */
  void compact_insertion_order();
};

} // namespace meta