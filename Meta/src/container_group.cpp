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

  bind_container(key, *it->second);

  return *it->second;
}

void ContainerGroup::bind_container(const std::string  &key,
                                    AttributeContainer &container)
{
  Logger::log()->trace("ContainerGroup::bind_container: key = {}", key);

  auto &conns = container_connections_[key];

  // Subscribe to attribute_added
  conns.push_back(container.attribute_added.subscribe(
      [this, key](AbstractAttribute &attr)
      {
        bind_attribute(key, attr);
        if (is_synchronized(attr.name()))
        {
          if (!is_synchronizing_)
          {
            // Find a source container that has this attribute
            AbstractAttribute *source = nullptr;
            if (current_ && current_ != find(key))
            {
              source = current_->find(attr.name());
            }
            if (!source)
            {
              for (const auto &cname : insertion_order_)
              {
                if (cname == key) continue;
                auto *c = find(cname);
                if (c && (source = c->find(attr.name()))) break;
              }
            }
            if (source && source->type() == attr.type())
            {
              is_synchronizing_ = true;
              attr.set_from_any(source->to_any());
              is_synchronizing_ = false;
            }
          }
        }
      }));

  // Bind any attributes already in the container
  for (auto &attr : container)
  {
    bind_attribute(key, *attr.second);
  }
}

void ContainerGroup::bind_attribute(const std::string &container_key,
                                    AbstractAttribute &attr)
{
  auto &conns = container_connections_[container_key];
  conns.push_back(attr.value_changed_event.subscribe(
      [this, attr_name = attr.name()](AbstractAttribute &source)
      {
        if (is_synchronizing_) return;
        if (!is_synchronized(attr_name)) return;

        is_synchronizing_ = true;
        sync_attribute_across_containers(attr_name, source);
        is_synchronizing_ = false;
      }));
}

void ContainerGroup::sync_attribute_across_containers(const std::string &key,
                                                      AbstractAttribute &source)
{
  std::any        val = source.to_any();
  std::type_index src_type = source.type();

  for (auto &[cname, container] : containers_)
  {
    if (!container) continue;
    auto *target_attr = container->find(key);
    if (!target_attr || target_attr == &source) continue;

    if (target_attr->type() == src_type)
    {
      target_attr->set_from_any(val);
    }
    else
    {
      Logger::log()->warn("ContainerGroup::sync_attribute: type mismatch for "
                          "attribute '{}' in container '{}'",
                          key,
                          cname);
    }
  }
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

  container_connections_.erase(key);
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
  container_connections_.clear();
  containers_.clear();
  insertion_order_.clear();
  synchronized_attributes_.clear();
  current_ = nullptr;
}

std::vector<std::string> ContainerGroup::shared_attributes() const
{
  std::unordered_map<std::string, std::unordered_map<std::type_index, size_t>>
                           counts;
  std::vector<std::string> order;

  for (const auto &cname : insertion_order_)
  {
    auto it = containers_.find(cname);
    if (it == containers_.end() || !it->second) continue;

    for (const auto &attr_name : it->second->insertion_order())
    {
      const auto *attr = it->second->find(attr_name);
      if (!attr) continue;

      if (!counts.contains(attr_name))
      {
        order.push_back(attr_name);
      }
      counts[attr_name][attr->type()]++;
    }
  }

  std::vector<std::string> result;
  for (const auto &attr_name : order)
  {
    for (const auto &[type, count] : counts[attr_name])
    {
      if (count >= 2)
      {
        result.push_back(attr_name);
        break;
      }
    }
  }
  return result;
}

bool ContainerGroup::is_shared(const std::string &key) const
{
  std::unordered_map<std::type_index, size_t> type_counts;
  for (const auto &[_, container] : containers_)
  {
    if (!container) continue;
    if (const auto *attr = container->find(key))
    {
      type_counts[attr->type()]++;
      if (type_counts[attr->type()] >= 2)
      {
        return true;
      }
    }
  }
  return false;
}

void ContainerGroup::set_synchronized(const std::string &key, bool synchronize)
{
  Logger::log()->trace("ContainerGroup::set_synchronized: key='{}', sync={}",
                       key,
                       synchronize);

  if (synchronize)
  {
    synchronized_attributes_.insert(key);
    sync_attribute(key);
  }
  else
  {
    synchronized_attributes_.erase(key);
  }
}

bool ContainerGroup::is_synchronized(const std::string &key) const
{
  return synchronized_attributes_.contains(key);
}

const std::unordered_set<std::string> &ContainerGroup::synchronized_attributes()
    const
{
  return synchronized_attributes_;
}

void ContainerGroup::synchronize_all(bool synchronize)
{
  Logger::log()->trace("ContainerGroup::synchronize_all: sync={}", synchronize);

  if (synchronize)
  {
    for (const auto &attr_name : shared_attributes())
    {
      set_synchronized(attr_name, true);
    }
  }
  else
  {
    clear_synchronizations();
  }
}

void ContainerGroup::clear_synchronizations()
{
  Logger::log()->trace("ContainerGroup::clear_synchronizations");
  synchronized_attributes_.clear();
}

void ContainerGroup::sync_attribute(const std::string &key)
{
  Logger::log()->trace("ContainerGroup::sync_attribute: key='{}'", key);

  AbstractAttribute *source = nullptr;
  if (current_)
  {
    source = current_->find(key);
  }

  if (!source)
  {
    for (const auto &cname : insertion_order_)
    {
      auto it = containers_.find(cname);
      if (it != containers_.end() && it->second)
      {
        if ((source = it->second->find(key)))
        {
          break;
        }
      }
    }
  }

  if (source)
  {
    is_synchronizing_ = true;
    sync_attribute_across_containers(key, *source);
    is_synchronizing_ = false;
  }
}

nlohmann::json ContainerGroup::json_to(SerializationMode mode) const
{
  Logger::log()->trace("ContainerGroup::json_to");

  nlohmann::json j = nlohmann::json::object();

  if (auto current_name = current_container_name())
  {
    j["current"] = *current_name;
  }

  if (!synchronized_attributes_.empty())
  {
    j["synchronized_attributes"] = synchronized_attributes_;
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
    if (name == "current" || name == "synchronized_attributes")
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

  if (j.contains("synchronized_attributes") &&
      j["synchronized_attributes"].is_array())
  {
    for (const auto &attr_val : j["synchronized_attributes"])
    {
      if (attr_val.is_string())
      {
        set_synchronized(attr_val.get<std::string>(), true);
      }
    }
  }
}

size_t ContainerGroup::size() const { return containers_.size(); }

} // namespace meta
