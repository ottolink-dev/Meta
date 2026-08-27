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
#include <unordered_set>
#include <vector>

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

  /// Returns the container names in insertion order.
  const std::vector<std::string> &insertion_order() const;

  /**
   * @brief Set the active container.
   * @throws std::runtime_error if the container does not exist.
   */
  void set_current(const std::string &key);

  /// Clear all containers.
  void clear();

  // -------------------------------------------------------------------------
  // Attribute Synchronization
  // -------------------------------------------------------------------------

  /**
   * @brief Detects attribute keys shared across at least two containers with
   *        matching types.
   * @return Vector of shared attribute keys in insertion order.
   */
  std::vector<std::string> shared_attributes() const;

  /**
   * @brief Checks whether an attribute key is shared across at least two
   *        containers with matching types.
   * @param key Attribute identifier.
   * @return true if shared, false otherwise.
   */
  bool is_shared(const std::string &key) const;

  /**
   * @brief Enable or disable synchronization for an attribute key across
   *        containers in this group.
   *
   * When enabled, the attribute's current value (from the active container, or
   * the first container containing it) is propagated to all matching
   * containers, and subsequent updates in any container are kept in sync.
   *
   * @param key Attribute identifier.
   * @param synchronize Whether to synchronize.
   */
  void set_synchronized(const std::string &key, bool synchronize = true);

  /**
   * @brief Check whether an attribute is marked for synchronization.
   * @param key Attribute identifier.
   * @return true if synchronized.
   */
  bool is_synchronized(const std::string &key) const;

  /**
   * @brief Get the set of currently synchronized attribute keys.
   * @return Constant reference to the set of synchronized attribute names.
   */
  const std::unordered_set<std::string> &synchronized_attributes() const;

  /**
   * @brief Synchronizes all detected shared attributes across containers.
   * @param synchronize Whether to enable or disable synchronization for all.
   */
  void synchronize_all(bool synchronize = true);

  /// Disables synchronization for all attributes.
  void clear_synchronizations();

  /**
   * @brief Manually propagate the value of an attribute from the active
   *        container (or first containing container) to all other matching
   *        containers in the group.
   * @param key Attribute identifier.
   */
  void sync_attribute(const std::string &key);

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
   * @param exclude_snapshot_manager Whether to exclude snapshot manager when
   * deserializing individual containers.
   */
  void json_from(const nlohmann::json &j, bool exclude_snapshot_manager)
  {
    json_from(j, SerializationMode::full, exclude_snapshot_manager);
  }

  /**
   * @brief Deserializes the container group from JSON.
   * @param j JSON object containing the container group data.
   * @param mode Serialization mode (full or state).
   * @param exclude_snapshot_manager Whether to exclude snapshot manager when
   * deserializing individual containers.
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

  std::unordered_set<std::string> synchronized_attributes_;
  bool                            is_synchronizing_ = false;

  // Track event connections per container
  std::unordered_map<std::string, std::vector<EventConnection>>
      container_connections_;

  void bind_container(const std::string &key, AttributeContainer &container);
  void bind_attribute(const std::string &container_key,
                      AbstractAttribute &attr);
  void sync_attribute_across_containers(const std::string &key,
                                        AbstractAttribute &source);

  /**
   * @brief Removes stale entries from insertion_order_ that no longer exist
   *        in attributes_ (e.g. after external erasure or clear).
   */
  void compact_insertion_order();
};

} // namespace meta