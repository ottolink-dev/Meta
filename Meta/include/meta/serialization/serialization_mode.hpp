/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once

namespace meta
{

/**
 * @brief Controls the scope and behavior of attribute serialization.
 */
enum class SerializationMode
{
  /// Full serialization: types, values, metadata, and state (default).
  full,

  /// State-only serialization: only values and state container, no metadata.
  state
};

} // namespace meta
