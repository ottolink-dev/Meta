/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

/**
 * @file attribute_traits_std.inl
 * @brief AttributeTraits specializations for standard library string types.
 * @copyright Copyright (c) 2026
 */
#pragma once
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace meta
{

/// Traits specialization for std::string serialization and formatting.
template <> struct AttributeTraits<std::string>
{
  static std::string to_string(const std::string &v) { return v; }

  static nlohmann::json json_to(const std::string &v) { return v; }

  static std::string json_from(const nlohmann::json &j)
  {
    return j.get<std::string>();
  }
};

/**
 * @brief Traits specialization for std::filesystem::path serialization and
 * formatting.
 */
template <> struct AttributeTraits<std::filesystem::path>
{
  static std::string to_string(const std::filesystem::path &v)
  {
    return v.string();
  }

  static nlohmann::json json_to(const std::filesystem::path &v) { return v; }

  static std::filesystem::path json_from(const nlohmann::json &j)
  {
    return j.get<std::filesystem::path>();
  }
};

// ---------------------------
//  Generic vector traits
// ---------------------------

/// Traits specialization for std::vector serialization and formatting.
template <typename T> struct AttributeTraits<std::vector<T>>
{
  static std::string to_string(const std::vector<T> &v)
  {
    std::ostringstream oss;

    for (size_t i = 0; i < v.size(); ++i)
    {
      if (i != 0) oss << ", ";

      oss << AttributeTraits<T>::to_string(v[i]);
    }

    return oss.str();
  }

  static nlohmann::json json_to(const std::vector<T> &v)
  {
    nlohmann::json j = nlohmann::json::array();

    for (const auto &x : v)
      j.push_back(AttributeTraits<T>::json_to(x));

    return j;
  }

  static std::vector<T> json_from(const nlohmann::json &j)
  {
    std::vector<T> v;
    v.reserve(j.size());

    for (const auto &x : j)
      v.push_back(AttributeTraits<T>::json_from(x));

    return v;
  }
};

/**
 * @brief Traits specialization for std::vector of std::pair serialization and
 * formatting.
 */
template <typename T, typename U>
struct AttributeTraits<std::vector<std::pair<T, U>>>
{
  using value_type = std::vector<std::pair<T, U>>;

  static std::string to_string(const value_type &v)
  {
    std::ostringstream oss;

    for (size_t i = 0; i < v.size(); ++i)
    {
      if (i != 0) oss << ", ";

      oss << "(" << AttributeTraits<T>::to_string(v[i].first) << ": "
          << AttributeTraits<U>::to_string(v[i].second) << ")";
    }

    return oss.str();
  }

  static nlohmann::json json_to(const value_type &v)
  {
    nlohmann::json j = nlohmann::json::array();

    for (const auto &p : v)
    {
      j.push_back({{"value", AttributeTraits<T>::json_to(p.first)},
                   {"label", AttributeTraits<U>::json_to(p.second)}});
    }

    return j;
  }

  static value_type json_from(const nlohmann::json &j)
  {
    value_type result;
    result.reserve(j.size());

    for (const auto &el : j)
    {
      result.emplace_back(AttributeTraits<T>::json_from(el.at("value")),
                          AttributeTraits<U>::json_from(el.at("label")));
    }

    return result;
  }
};

// ---------------------------
//  Generic map trait
// ---------------------------

/// Traits specialization for std::map serialization and formatting.
template <typename K, typename V> struct AttributeTraits<std::map<K, V>>
{
  using value_type = std::map<K, V>;

  static std::string to_string(const value_type &m)
  {
    std::ostringstream oss;

    bool first = true;
    for (const auto &[k, v] : m)
    {
      if (!first) oss << ", ";

      first = false;

      oss << AttributeTraits<K>::to_string(k) << ": "
          << AttributeTraits<V>::to_string(v);
    }

    return oss.str();
  }

  static nlohmann::json json_to(const value_type &m)
  {
    nlohmann::json j = nlohmann::json::array();

    for (const auto &[k, v] : m)
    {
      j.push_back({{"key", AttributeTraits<K>::json_to(k)},
                   {"value", AttributeTraits<V>::json_to(v)}});
    }

    return j;
  }

  static value_type json_from(const nlohmann::json &j)
  {
    value_type m;

    for (const auto &el : j)
    {
      m.emplace(AttributeTraits<K>::json_from(el.at("key")),
                AttributeTraits<V>::json_from(el.at("value")));
    }

    return m;
  }
};

// ---------------------------
//  Generic unordered map trait
// ---------------------------

/**
 * @brief Traits specialization for std::unordered_map serialization and
 * formatting.
 */
template <typename K, typename V>
struct AttributeTraits<std::unordered_map<K, V>>
{
  using value_type = std::unordered_map<K, V>;

  static std::string to_string(const value_type &m)
  {
    std::ostringstream oss;

    bool first = true;
    for (const auto &[k, v] : m)
    {
      if (!first) oss << ", ";

      first = false;

      oss << AttributeTraits<K>::to_string(k) << ": "
          << AttributeTraits<V>::to_string(v);
    }

    return oss.str();
  }

  static nlohmann::json json_to(const value_type &m)
  {
    nlohmann::json j = nlohmann::json::array();

    for (const auto &[k, v] : m)
    {
      j.push_back({{"key", AttributeTraits<K>::json_to(k)},
                   {"value", AttributeTraits<V>::json_to(v)}});
    }

    return j;
  }

  static value_type json_from(const nlohmann::json &j)
  {
    value_type m;

    for (const auto &el : j)
    {
      m.emplace(AttributeTraits<K>::json_from(el.at("key")),
                AttributeTraits<V>::json_from(el.at("value")));
    }

    return m;
  }
};

} // namespace meta