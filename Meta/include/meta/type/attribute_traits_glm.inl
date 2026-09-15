/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */

/**
 * @file attribute_traits_glm.inl
 * @copyright Copyright (c) 2026
 */
#pragma once
#include <string>

#include <glm/glm.hpp>

#include <nlohmann/json.hpp>

namespace meta
{

/// Traits specialization for glm::vec2 serialization and formatting.
template <> struct AttributeTraits<glm::vec2>
{
  static std::string to_string(const glm::vec2 &v)
  {
    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
  }

  static nlohmann::json json_to(const glm::vec2 &v)
  {
    return {{"x", v.x}, {"y", v.y}};
  }

  static glm::vec2 json_from(const nlohmann::json &j)
  {
    if (j.is_array() && j.size() >= 2)
    {
      return {j[0].get<float>(), j[1].get<float>()};
    }
    return {j.at("x").get<float>(), j.at("y").get<float>()};
  }
};

/// Traits specialization for glm::vec3 serialization and formatting.
template <> struct AttributeTraits<glm::vec3>
{
  static std::string to_string(const glm::vec3 &v)
  {
    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " +
           std::to_string(v.z) + ")";
  }

  static nlohmann::json json_to(const glm::vec3 &v)
  {
    return {{"x", v.x}, {"y", v.y}, {"z", v.z}};
  }

  static glm::vec3 json_from(const nlohmann::json &j)
  {
    if (j.is_array() && j.size() >= 3)
    {
      return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
    }
    return {j.at("x").get<float>(),
            j.at("y").get<float>(),
            j.at("z").get<float>()};
  }
};

/// Traits specialization for glm::vec4 serialization and formatting.
template <> struct AttributeTraits<glm::vec4>
{
  static std::string to_string(const glm::vec4 &v)
  {
    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " +
           std::to_string(v.z) + ", " + std::to_string(v.w) + ")";
  }

  static nlohmann::json json_to(const glm::vec4 &v)
  {
    return {{"x", v.x}, {"y", v.y}, {"z", v.z}, {"w", v.w}};
  }

  static glm::vec4 json_from(const nlohmann::json &j)
  {
    if (j.is_array() && j.size() >= 4)
    {
      return {j[0].get<float>(),
              j[1].get<float>(),
              j[2].get<float>(),
              j[3].get<float>()};
    }
    return {j.at("x").get<float>(),
            j.at("y").get<float>(),
            j.at("z").get<float>(),
            j.at("w").get<float>()};
  }
};

/// Traits specialization for glm::ivec2 serialization and formatting.
template <> struct AttributeTraits<glm::ivec2>
{
  static std::string to_string(const glm::ivec2 &v)
  {
    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
  }

  static nlohmann::json json_to(const glm::ivec2 &v)
  {
    return {{"x", v.x}, {"y", v.y}};
  }

  static glm::ivec2 json_from(const nlohmann::json &j)
  {
    if (j.is_array() && j.size() >= 2)
    {
      return {j[0].get<int>(), j[1].get<int>()};
    }
    return {j.at("x").get<int>(), j.at("y").get<int>()};
  }
};

/// Traits specialization for glm::ivec3 serialization and formatting.
template <> struct AttributeTraits<glm::ivec3>
{
  static std::string to_string(const glm::ivec3 &v)
  {
    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " +
           std::to_string(v.z) + ")";
  }

  static nlohmann::json json_to(const glm::ivec3 &v)
  {
    return {{"x", v.x}, {"y", v.y}, {"z", v.z}};
  }

  static glm::ivec3 json_from(const nlohmann::json &j)
  {
    if (j.is_array() && j.size() >= 3)
    {
      return {j[0].get<int>(), j[1].get<int>(), j[2].get<int>()};
    }
    return {j.at("x").get<int>(), j.at("y").get<int>(), j.at("z").get<int>()};
  }
};

/// Traits specialization for glm::ivec4 serialization and formatting.
template <> struct AttributeTraits<glm::ivec4>
{
  static std::string to_string(const glm::ivec4 &v)
  {
    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " +
           std::to_string(v.z) + ", " + std::to_string(v.w) + ")";
  }

  static nlohmann::json json_to(const glm::ivec4 &v)
  {
    return {{"x", v.x}, {"y", v.y}, {"z", v.z}, {"w", v.w}};
  }

  static glm::ivec4 json_from(const nlohmann::json &j)
  {
    if (j.is_array() && j.size() >= 4)
    {
      return {j[0].get<int>(),
              j[1].get<int>(),
              j[2].get<int>(),
              j[3].get<int>()};
    }
    return {j.at("x").get<int>(),
            j.at("y").get<int>(),
            j.at("z").get<int>(),
            j.at("w").get<int>()};
  }
};

} // namespace meta