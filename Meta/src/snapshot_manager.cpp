/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/serialization/snapshot_manager.hpp"
#include "meta/logger.hpp"

namespace meta
{

void SnapshotManager::save(const std::string    &name,
                           const nlohmann::json &snapshot)
{
  Logger::log()->trace("SnapshotManager::save: '{}'", name);
  snapshots_[name] = snapshot;
  snapshots_modified.notify();
}

void SnapshotManager::clear()
{
  Logger::log()->trace("SnapshotManager::clear ({} snapshots)",
                       snapshots_.size());
  snapshots_.clear();
  snapshots_modified.notify();
}

void SnapshotManager::erase(const std::string &name)
{
  Logger::log()->trace("SnapshotManager::erase: '{}'", name);
  snapshots_.erase(name);
  snapshots_modified.notify();
}

bool SnapshotManager::has(const std::string &name) const
{
  Logger::log()->trace("SnapshotManager::has: '{}'", name);
  return snapshots_.contains(name);
}

bool SnapshotManager::empty() const { return snapshots_.empty(); }

bool SnapshotManager::json_from(const nlohmann::json &j)
{
  try
  {
    if (!j.is_object()) return false;

    if (!j.contains("snapshots") || !j["snapshots"].is_object()) return false;

    const auto &snapshots_json = j["snapshots"];

    // clear current state before loading
    snapshots_.clear();

    for (auto it = snapshots_json.begin(); it != snapshots_json.end(); ++it)
    {
      if (!it.value().is_object() && !it.value().is_array() &&
          !it.value().is_primitive())
      {
        // skip invalid entries
        continue;
      }

      snapshots_[it.key()] = it.value();
    }

    snapshots_modified.notify();

    return true;
  }
  catch (...)
  {
    // failsafe: do not partially corrupt state
    snapshots_.clear();
    return false;
  }
}

nlohmann::json SnapshotManager::json_to() const
{
  nlohmann::json j;

  try
  {
    j["snapshots"] = nlohmann::json::object();

    for (const auto &it : snapshots_)
    {
      // ensure we store only valid JSON values
      j["snapshots"][it.first] = it.second;
    }
  }
  catch (...)
  {
    // failsafe: return minimal valid structure
    return nlohmann::json{{"snapshots", nlohmann::json::object()}};
  }

  return j;
}

const nlohmann::json &SnapshotManager::load(const std::string &name) const
{
  Logger::log()->trace("SnapshotManager::load: '{}'", name);
  return snapshots_.at(name);
}

std::vector<std::string> SnapshotManager::names() const
{
  Logger::log()->trace("SnapshotManager::names: {} entries", snapshots_.size());

  std::vector<std::string> result;
  result.reserve(snapshots_.size());

  for (const auto &[name, _] : snapshots_)
    result.push_back(name);

  return result;
}

} // namespace meta
