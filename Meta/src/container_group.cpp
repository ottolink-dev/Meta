/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/core/container_group.hpp"
#include "meta/logger.hpp"

namespace meta
{

AttributeContainer &ContainerGroup::add(const std::string &key)
{
  Logger::log()->trace("ContainerGroup::add: key = {}", key);

  auto [it, inserted] = containers_.try_emplace(
      key,
      std::make_unique<AttributeContainer>());

  if (!inserted)
  {
    Logger::log()->trace("ContainerGroup::add: key already exists = {}", key);
    throw std::runtime_error("Container already exists: " + key);
  }

  if (!current_)
  {
    Logger::log()->trace("ContainerGroup::add: setting current to '{}'", key);
    current_ = it->second.get();
  }

  insertion_order_.push_back(key);

  return *it->second;
}

void ContainerGroup::compact_insertion_order()
{
  Logger::log()->trace("ContainerGroup::compact_insertion_order");

  insertion_order_.erase(std::remove_if(insertion_order_.begin(),
                                        insertion_order_.end(),
                                        [this](const std::string &name) {
                                          return !containers_.contains(name);
                                        }),
                         insertion_order_.end());
}

const std::unordered_map<std::string, std::unique_ptr<AttributeContainer>> &
ContainerGroup::containers() const
{
  return containers_;
}

bool ContainerGroup::contains(const std::string &key) const
{
  return containers_.contains(key);
}

AttributeContainer &ContainerGroup::current()
{
  if (!current_)
  {
    Logger::log()->trace("ContainerGroup::current: no current container");
    throw std::runtime_error("No current container selected");
  }

  return *current_;
}

const AttributeContainer &ContainerGroup::current() const
{
  if (!current_)
  {
    Logger::log()->trace("ContainerGroup::current const: no current container");
    throw std::runtime_error("No current container selected");
  }

  return *current_;
}

std::optional<std::string> ContainerGroup::current_container_name() const
{
  if (!current_) return std::nullopt;

  for (const auto &[key, container] : containers_)
    if (container.get() == current_) return key;

  return std::nullopt;
}

bool ContainerGroup::erase(const std::string &key)
{
  Logger::log()->trace("ContainerGroup::erase: key = {}", key);

  auto it = containers_.find(key);

  if (it == containers_.end())
  {
    Logger::log()->trace("ContainerGroup::erase: key not found = {}", key);
    return false;
  }

  const bool was_current = (current_ == it->second.get());

  containers_.erase(it);

  if (was_current)
  {
    Logger::log()->trace("ContainerGroup::erase: current container erased");
    current_ = nullptr;
  }

  if (!current_ && !containers_.empty())
  {
    current_ = containers_.begin()->second.get();
    Logger::log()->trace("ContainerGroup::erase: new current auto-selected");
  }

  compact_insertion_order();

  return true;
}

AttributeContainer *ContainerGroup::find(const std::string &key)
{
  auto it = containers_.find(key);
  return it != containers_.end() ? it->second.get() : nullptr;
}

const AttributeContainer *ContainerGroup::find(const std::string &key) const
{
  auto it = containers_.find(key);
  return it != containers_.end() ? it->second.get() : nullptr;
}

const std::vector<std::string> &ContainerGroup::insertion_order() const
{
  return insertion_order_;
}

void ContainerGroup::set_current(const std::string &key)
{
  Logger::log()->trace("ContainerGroup::set_current: key = {}", key);

  auto *container = find(key);

  if (!container)
  {
    Logger::log()->trace("ContainerGroup::set_current: invalid key = {}", key);
    throw std::runtime_error("Container does not exist: " + key);
  }

  current_ = container;
  Logger::log()->trace("ContainerGroup::set_current: success = {}", key);
}

void ContainerGroup::clear()
{
  Logger::log()->trace("ContainerGroup::clear");
  containers_.clear();
  insertion_order_.clear();
  current_ = nullptr;
}

nlohmann::json ContainerGroup::json_to(SerializationMode mode) const
{
  Logger::log()->trace("ContainerGroup::json_to");

  nlohmann::json j = nlohmann::json::object();

  if (auto current_name = current_container_name())
  {
    j["current"] = *current_name;
  }

  nlohmann::json containers_json = nlohmann::json::object();
  for (const auto &name : insertion_order_)
  {
    auto it = containers_.find(name);
    if (it != containers_.end() && it->second)
    {
      containers_json[name] = it->second->json_to(mode);
    }
  }

  for (const auto &[name, container] : containers_)
  {
    if (!containers_json.contains(name) && container)
    {
      containers_json[name] = container->json_to(mode);
    }
  }

  j["containers"] = std::move(containers_json);

  return j;
}

void ContainerGroup::json_from(const nlohmann::json &j,
                               SerializationMode     mode,
                               bool                  exclude_snapshot_manager)
{
  Logger::log()->trace("ContainerGroup::json_from");

  if (!j.is_object())
  {
    Logger::log()->error("ContainerGroup::json_from: expected JSON object");
    return;
  }

  const nlohmann::json *containers_json = nullptr;

  if (j.contains("containers") && j["containers"].is_object())
  {
    containers_json = &j["containers"];
  }
  else
  {
    containers_json = &j;
  }

  for (const auto &[name, container_val] : containers_json->items())
  {
    if (name == "current")
    {
      continue;
    }

    if (!container_val.is_object())
    {
      Logger::log()->warn("ContainerGroup::json_from: skipping container '{}' "
                          "because value is not an object",
                          name);
      continue;
    }

    auto it = containers_.find(name);

    if (it == containers_.end())
    {
      if (mode == SerializationMode::state)
      {
        Logger::log()->trace("ContainerGroup::json_from: skipping undeclared "
                             "container '{}' in state mode",
                             name);
        continue;
      }

      Logger::log()->trace("ContainerGroup::json_from: creating container '{}'",
                           name);
      AttributeContainer &new_container = add(name);
      new_container.json_from(container_val, mode, exclude_snapshot_manager);
    }
    else
    {
      Logger::log()->trace("ContainerGroup::json_from: updating container '{}'",
                           name);
      it->second->json_from(container_val, mode, exclude_snapshot_manager);
    }
  }

  if (j.contains("current") && j["current"].is_string())
  {
    const std::string current_name = j["current"].get<std::string>();
    if (contains(current_name))
    {
      set_current(current_name);
    }
    else
    {
      Logger::log()->warn(
          "ContainerGroup::json_from: current container '{}' not found",
          current_name);
    }
  }
}

size_t ContainerGroup::size() const { return containers_.size(); }

} // namespace meta

