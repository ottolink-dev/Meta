/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

/**
 * @file meta_object.hpp
 * @brief Base class providing owned metadata storage for reflection-enabled
 *        objects.
 *
 * MetaObject is a lightweight utility class that gives derived classes
 * a fully owned AttributeContainer used to store arbitrary runtime
 * metadata.
 *
 * This metadata system is used for GUI reflection, serialization hints,
 * editor configuration, and user-defined extensions.
 *
 * The metadata is fully owned (no sharing), ensuring deterministic
 * behavior and avoiding cross-instance side effects.
 *
 * @copyright Copyright (c) 2026
 */
#pragma once
#include <memory>

namespace meta
{

class AttributeContainer;

/**
 * @brief Base class enabling runtime metadata attachment.
 *
 * MetaObject provides a unified way to attach arbitrary key/value metadata
 * to any object in the Meta framework.
 *
 * Typical uses include:
 * - GUI reflection hints (slider ranges, widgets, grouping)
 * - Serialization annotations
 * - Editor-specific configuration
 * - User-defined extensions
 *
 * Ownership model:
 * - Metadata is fully owned by the object
 * - No shared or global state
 * - Lifetime tied to the object
 */
class MetaObject
{
public:
  /// Construct a MetaObject with lazy metadata and state containers.
  MetaObject() = default;

  /// Default destructor.
  virtual ~MetaObject() = default;

  /**
   * @brief Access mutable metadata container (allocated on-demand).
   * @return Reference to the internal AttributeContainer.
   */
  AttributeContainer &metadata();

  /**
   * @brief Access immutable metadata container.
   * @return Const reference to the internal AttributeContainer (or static empty
   * container if unallocated).
   */
  const AttributeContainer &metadata() const;

  /**
   * @brief Access mutable state container (allocated on-demand).
   * @return Reference to the internal AttributeContainer.
   */
  AttributeContainer &state();

  /**
   * @brief Access immutable state container.
   * @return Const reference to the internal AttributeContainer (or static empty
   * container if unallocated).
   */
  const AttributeContainer &state() const;

  /**
   * @brief Checks if metadata container has been allocated and is non-empty.
   */
  bool has_metadata() const noexcept;

  /**
   * @brief Checks if state container has been allocated and is non-empty.
   */
  bool has_state() const noexcept;

private:
  /// Owned metadata container storing arbitrary attributes (lazily allocated).
  mutable std::unique_ptr<AttributeContainer> metadata_{nullptr};

  /// Owned state container storing runtime/user state attributes (lazily
  /// allocated).
  mutable std::unique_ptr<AttributeContainer> state_{nullptr};
};

} // namespace meta