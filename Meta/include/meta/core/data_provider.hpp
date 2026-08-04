/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <any>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "meta/type/attribute_traits.hpp"
#include "meta/type/type_name.hpp"

namespace meta
{

struct Any : std::any
{
  using std::any::any;

  template <typename T>
  const T* get() const
  {
    return std::any_cast<T>(this);
  }

  template <typename T>
  T* get()
  {
    return std::any_cast<T>(this);
  }
};

/// Host-supplied callback returning fresh display data on each call. Non-serializable.
using DataProvider = std::function<Any()>;

/// No-op traits: a DataProvider carries runtime state that must not be serialized.
template <> struct AttributeTraits<DataProvider>
{
  static std::string   to_string(const DataProvider &) { return "<data_provider>"; }

  static nlohmann::json json_to(const DataProvider &) { return nullptr; }
  
  static DataProvider   json_from(const nlohmann::json &) { return {}; }
};

} // namespace meta

META_DEFINE_TYPE_NAME(meta::DataProvider);
