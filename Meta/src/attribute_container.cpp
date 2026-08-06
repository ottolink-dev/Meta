/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <set>
#include <utility>

#include <nlohmann/json.hpp>

#include "meta/core/attribute_container.hpp"
#include "meta/core/data_provider.hpp"
#include "meta/logger.hpp"
#include "meta/serialization/attribute_factory.hpp"

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
#include "meta/ext/color_gradient/color_gradient.hpp"
#endif

namespace meta
{

AttrIterator AttributeContainer::begin() { return attributes_.begin(); }

ConstAttrIterator AttributeContainer::begin() const
{
  return attributes_.begin();
}

ConstAttrIterator AttributeContainer::cbegin() const
{
  return attributes_.cbegin();
}

ConstAttrIterator AttributeContainer::cend() const
{
  return attributes_.cend();
}

void AttributeContainer::clear()
{
  Logger::log()->trace("AttributeContainer::clear");
  attributes_.clear();
  compact_insertion_order();
}

void AttributeContainer::compact_insertion_order()
{
  Logger::log()->trace("AttributeContainer::compact_insertion_order");

  insertion_order_.erase(std::remove_if(insertion_order_.begin(),
                                        insertion_order_.end(),
                                        [this](const std::string &name) {
                                          return !attributes_.contains(name);
                                        }),
                         insertion_order_.end());
}

bool AttributeContainer::contains(const std::string &name) const
{
  return attributes_.contains(name);
}

bool AttributeContainer::contains_all_keys(const std::vector<std::string> &keys)
{
  Logger::log()->trace("AttributeContainer::contains_all_keys: {} keys",
                       keys.size());

  for (const auto &key : keys)
    if (attributes_.find(key) == attributes_.end()) return false;

  return true;
}

bool AttributeContainer::empty() const noexcept { return attributes_.empty(); }

AttrIterator AttributeContainer::end() { return attributes_.end(); }

ConstAttrIterator AttributeContainer::end() const { return attributes_.end(); }

AbstractAttribute *AttributeContainer::find(const std::string &name)
{
  auto it = attributes_.find(name);
  return it == attributes_.end() ? nullptr : it->second.get();
}

const AbstractAttribute *AttributeContainer::find(const std::string &name) const
{
  auto it = attributes_.find(name);
  return it == attributes_.end() ? nullptr : it->second.get();
}

const std::vector<std::string> &AttributeContainer::insertion_order() const
{
  return insertion_order_;
}

bool AttributeContainer::set_insertion_order(
    const std::vector<std::string> &order)
{
  if (order.size() != insertion_order_.size()) return false;

  for (const auto &k : order)
    if (attributes_.find(k) == attributes_.end()) return false;

  // reject duplicates
  std::set<std::string> uniq(order.begin(), order.end());
  if (uniq.size() != order.size()) return false;

  insertion_order_ = order;
  return true;
}

void AttributeContainer::json_from(const nlohmann::json &j,
                                   bool exclude_snapshot_manager)
{
  Logger::log()->trace("AttributeContainer::json_from: {} entries", j.size());

  if (!j.is_object())
  {
    Logger::log()->error("AttributeContainer::json_from: expected JSON object");
    return;
  }

  Logger::log()->trace("AttributeContainer::json_from: {} entries", j.size());

  for (auto &[name, value] : j.items())
  {
    // Reserved entry
    if (name == "snapshot_manager") continue;

    if (!value.is_object())
    {
      Logger::log()->warn("json_from: skipping attribute '{}' because its "
                          "value is not an object",
                          name);
      continue;
    }

    auto it = attributes_.find(name);

    if (it == attributes_.end())
    {
      if (!value.contains("type") || !value["type"].is_string())
      {
        Logger::log()->warn(
            "json_from: cannot create attribute '{}' (missing or invalid "
            "'type' field)",
            name);
        continue;
      }

      const std::string attr_type = value["type"].get<std::string>();

      Logger::log()->trace("json_from: creating attribute '{}' of type '{}'",
                           name,
                           attr_type);

      auto new_attr = AttributeFactory::instance().create(attr_type, name);

      if (!new_attr)
      {
        Logger::log()->error("json_from: unknown attribute type '{}' for '{}'",
                             attr_type,
                             name);
        continue;
      }

      auto [insert_it, inserted] = attributes_.emplace(name,
                                                       std::move(new_attr));

      it = insert_it;
    }
    else
    {
      Logger::log()->trace("json_from: updating attribute '{}'", name);
    }

    try
    {
      it->second->json_from(value);
    }
    catch (const std::exception &e)
    {
      Logger::log()->error(
          "json_from: failed to deserialize attribute '{}': {}",
          name,
          e.what());
    }
  }

  if (!exclude_snapshot_manager && j.contains("snapshot_manager"))
  {
    try
    {
      snapshot_manager_.json_from(j["snapshot_manager"]);
    }
    catch (const std::exception &e)
    {
      Logger::log()->error(
          "json_from: failed to deserialize snapshot manager: {}",
          e.what());
    }
  }
}

nlohmann::json AttributeContainer::json_to() const
{
  Logger::log()->trace("AttributeContainer::json_to");

  nlohmann::json j = nlohmann::json::object();

  for (const auto &[name, attr] : attributes_)
  {
    if (attr->type() == std::type_index(typeid(meta::DataProvider)))
      continue; // non-serializable runtime provider
#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
    if (attr->type() == std::type_index(typeid(meta::GradientPresets)))
      continue; // non-serializable host configuration
#endif
    j[name] = attr->json_to();
  }

  if (!snapshot_manager_.empty())
  {
    j["snapshot_manager"] = snapshot_manager_.json_to();
  }

  return j;
}

std::size_t AttributeContainer::size() const noexcept
{
  return attributes_.size();
}

SnapshotManager &AttributeContainer::snapshot_manager()
{
  return snapshot_manager_;
}

const SnapshotManager &AttributeContainer::snapshot_manager() const
{
  return snapshot_manager_;
}

// --- Functions

void deserialize_metadata(AttributeContainer &m, const nlohmann::json &j)
{
  Logger::log()->trace("deserialize_metadata");
  m.json_from(j);
}

nlohmann::json serialize_metadata(const AttributeContainer &m)
{
  Logger::log()->trace("serialize_metadata");
  return m.json_to();
}

} // namespace meta
