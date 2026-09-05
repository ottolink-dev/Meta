/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <algorithm>
#include <cctype>
#include <fstream>

#include "meta/ext/color_gradient/gradient_library.hpp"
#include "meta/logger.hpp"

namespace meta {

namespace {

constexpr char kFileFormat[] = "meta.gradients";
constexpr int kFileVersion = 1;
constexpr char kDefaultName[] = "Gradient";

std::string trim(std::string_view text) {
  const auto not_space = [](unsigned char c) { return !std::isspace(c); };
  const auto begin = std::find_if(text.begin(), text.end(), not_space);
  const auto end = std::find_if(text.rbegin(), text.rend(), not_space).base();
  return begin < end ? std::string(begin, end) : std::string();
}

void sort_stops(std::vector<Stop> &stops) {
  std::stable_sort(
      stops.begin(), stops.end(),
      [](const Stop &a, const Stop &b) { return a.position < b.position; });
}

bool parse_color(const nlohmann::json &json, std::array<float, 4> &out) {
  if (!json.is_array() || json.size() < 3)
    return false;

  const std::size_t n = std::min<std::size_t>(4, json.size());
  std::array<float, 4> color = {0.f, 0.f, 0.f, 1.f};
  bool over_one = false;

  for (std::size_t k = 0; k < n; ++k) {
    if (!json[k].is_number())
      return false;
    color[k] = json[k].get<float>();
    if (color[k] > 1.f)
      over_one = true;
  }

  // 0-255 encoded colours (e.g. Hesiod's data/color_gradient.json)
  if (over_one)
    for (std::size_t k = 0; k < n; ++k)
      color[k] /= 255.f;

  for (float &v : color)
    v = std::clamp(v, 0.f, 1.f);

  out = color;
  return true;
}

std::optional<Preset> parse_preset(const nlohmann::json &json,
                                   std::string_view fallback_name) {
  if (!json.is_object())
    return std::nullopt;

  const nlohmann::json *stops_json = nullptr;
  if (json.contains("stops") && json["stops"].is_array())
    stops_json = &json["stops"];
  else if (json.contains("value") && json["value"].is_array())
    stops_json = &json["value"];

  if (!stops_json)
    return std::nullopt;

  Preset preset;
  if (json.contains("name") && json["name"].is_string())
    preset.name = trim(json["name"].get<std::string>());
  if (preset.name.empty())
    preset.name = std::string(fallback_name);

  for (const auto &s : *stops_json) {
    if (!s.is_object() || !s.contains("position") ||
        !s["position"].is_number() || !s.contains("color"))
      continue;

    std::array<float, 4> color;
    if (!parse_color(s["color"], color))
      continue;

    preset.stops.push_back(
        {std::clamp(s["position"].get<float>(), 0.f, 1.f), color});
  }

  if (preset.stops.size() < 2)
    return std::nullopt;

  sort_stops(preset.stops);
  return preset;
}

std::vector<Preset> parse_preset_list(const nlohmann::json &array,
                                      std::string_view fallback_name) {
  std::vector<Preset> out;
  std::size_t index = 0;

  for (const auto &g : array) {
    ++index;
    const std::string fallback =
        array.size() > 1
            ? std::string(fallback_name) + " " + std::to_string(index)
            : std::string(fallback_name);

    if (auto preset = parse_preset(g, fallback))
      out.push_back(std::move(*preset));
    else
      Logger::log()->warn(
          "parse_gradient_file: skipping invalid gradient entry #{}", index);
  }

  return out;
}

bool read_json_file(const std::filesystem::path &path, nlohmann::json &out) {
  std::ifstream file(path);
  if (!file) {
    Logger::log()->error("GradientLibrary: cannot open '{}'", path.string());
    return false;
  }

  try {
    file >> out;
  } catch (const std::exception &e) {
    Logger::log()->error("GradientLibrary: cannot parse '{}': {}",
                         path.string(), e.what());
    return false;
  }

  return true;
}

bool write_json_file(const std::filesystem::path &path,
                     const nlohmann::json &json) {
  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);

  std::ofstream file(path);
  if (!file) {
    Logger::log()->error("GradientLibrary: cannot write '{}'", path.string());
    return false;
  }

  file << json.dump(2) << '\n';
  return static_cast<bool>(file);
}

} // namespace

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

std::string_view to_string(GradientSort sort) {
  switch (sort) {
  case GradientSort::Name:
    return "name";
  case GradientSort::Luminance:
    return "luminance";
  case GradientSort::Hue:
    return "hue";
  case GradientSort::Default:
  default:
    return "default";
  }
}

std::optional<GradientSort> gradient_sort_from_string(std::string_view text) {
  for (GradientSort s : {GradientSort::Default, GradientSort::Name,
                         GradientSort::Luminance, GradientSort::Hue})
    if (text == to_string(s))
      return s;
  return std::nullopt;
}

nlohmann::json gradient_file_json(const std::vector<Preset> &presets) {
  nlohmann::json json;
  json["format"] = kFileFormat;
  json["version"] = kFileVersion;
  json["gradients"] = nlohmann::json::array();

  for (const auto &preset : presets) {
    nlohmann::json g;
    g["name"] = preset.name;
    g["stops"] = nlohmann::json::array();
    for (const auto &s : preset.stops)
      g["stops"].push_back({{"position", s.position}, {"color", s.color}});
    json["gradients"].push_back(std::move(g));
  }

  return json;
}

std::optional<std::vector<Preset>>
parse_gradient_file(const nlohmann::json &json,
                    std::string_view fallback_name) {
  std::vector<Preset> out;

  if (json.is_array())
    out = parse_preset_list(json, fallback_name);
  else if (json.is_object() && json.contains("gradients") &&
           json["gradients"].is_array())
    out = parse_preset_list(json["gradients"], fallback_name);
  else if (auto preset = parse_preset(json, fallback_name))
    out.push_back(std::move(*preset));

  if (out.empty())
    return std::nullopt;
  return out;
}

// ---------------------------------------------------------------------------
// GradientLibrary
// ---------------------------------------------------------------------------

GradientLibrary &GradientLibrary::instance() {
  static GradientLibrary library;
  return library;
}

void GradientLibrary::set_path(std::filesystem::path path) {
  path_ = std::move(path);
}

const std::filesystem::path &GradientLibrary::path() const { return path_; }

void GradientLibrary::set_autosave(bool on) { autosave_ = on; }

bool GradientLibrary::autosave() const { return autosave_; }

bool GradientLibrary::load() {
  if (path_.empty()) {
    Logger::log()->warn("GradientLibrary::load: no path set");
    return false;
  }

  std::error_code ec;
  if (!std::filesystem::exists(path_, ec)) {
    Logger::log()->trace("GradientLibrary::load: no file at '{}'",
                         path_.string());
    return false;
  }

  nlohmann::json json;
  if (!read_json_file(path_, json))
    return false;

  if (!json_from(json)) {
    Logger::log()->warn("GradientLibrary::load: '{}' is not a gradient library",
                        path_.string());
    return false;
  }

  Logger::log()->trace("GradientLibrary::load: {} presets from '{}'",
                       presets_.size(), path_.string());
  return true;
}

bool GradientLibrary::save() const {
  if (path_.empty()) {
    Logger::log()->trace("GradientLibrary::save: no path set, skipping");
    return false;
  }
  return write_json_file(path_, json_to());
}

const std::vector<Preset> &GradientLibrary::presets() const { return presets_; }

bool GradientLibrary::has(std::string_view name) const {
  return find(name) != nullptr;
}

const Preset *GradientLibrary::find(std::string_view name) const {
  const auto it =
      std::find_if(presets_.begin(), presets_.end(),
                   [name](const Preset &p) { return p.name == name; });
  return it == presets_.end() ? nullptr : &*it;
}

std::string GradientLibrary::add(Preset preset) {
  preset.name = unique_name(preset.name);
  sort_stops(preset.stops);

  Logger::log()->trace("GradientLibrary::add: '{}'", preset.name);

  presets_.push_back(std::move(preset));
  const std::string name = presets_.back().name;
  on_modified();
  return name;
}

bool GradientLibrary::update(std::string_view name, std::vector<Stop> stops) {
  const auto it =
      std::find_if(presets_.begin(), presets_.end(),
                   [name](const Preset &p) { return p.name == name; });
  if (it == presets_.end())
    return false;

  sort_stops(stops);
  it->stops = std::move(stops);

  Logger::log()->trace("GradientLibrary::update: '{}'", it->name);
  on_modified();
  return true;
}

bool GradientLibrary::rename(std::string_view from, std::string_view to) {
  const std::string new_name = trim(to);

  const auto it =
      std::find_if(presets_.begin(), presets_.end(),
                   [from](const Preset &p) { return p.name == from; });
  if (it == presets_.end() || new_name.empty())
    return false;
  if (new_name == it->name)
    return true;
  if (has(new_name))
    return false;

  const std::string old_name = it->name;
  it->name = new_name;

  for (auto &favorite : favorites_)
    if (favorite == old_name)
      favorite = new_name;

  Logger::log()->trace("GradientLibrary::rename: '{}' -> '{}'", old_name,
                       new_name);
  on_modified();
  return true;
}

bool GradientLibrary::remove(std::string_view name) {
  const auto it =
      std::find_if(presets_.begin(), presets_.end(),
                   [name](const Preset &p) { return p.name == name; });
  if (it == presets_.end())
    return false;

  const std::string removed = it->name; // `name` may alias it->name
  presets_.erase(it);
  favorites_.erase(std::remove(favorites_.begin(), favorites_.end(), removed),
                   favorites_.end());

  Logger::log()->trace("GradientLibrary::remove: '{}'", removed);
  on_modified();
  return true;
}

void GradientLibrary::clear() {
  Logger::log()->trace("GradientLibrary::clear ({} presets)", presets_.size());
  presets_.clear();
  favorites_.clear();
  on_modified();
}

std::string
GradientLibrary::unique_name(std::string_view base,
                             const std::vector<std::string> &reserved) const {
  std::string name = trim(base);
  if (name.empty())
    name = kDefaultName;

  const auto taken = [&](const std::string &candidate) {
    return has(candidate) || std::find(reserved.begin(), reserved.end(),
                                       candidate) != reserved.end();
  };

  if (!taken(name))
    return name;

  for (int i = 2;; ++i) {
    const std::string candidate = name + " (" + std::to_string(i) + ")";
    if (!taken(candidate))
      return candidate;
  }
}

bool GradientLibrary::is_favorite(std::string_view name) const {
  return std::find(favorites_.begin(), favorites_.end(), name) !=
         favorites_.end();
}

void GradientLibrary::set_favorite(std::string_view name, bool on) {
  const auto it = std::find(favorites_.begin(), favorites_.end(), name);
  const bool present = it != favorites_.end();
  if (on == present)
    return;

  if (on)
    favorites_.emplace_back(name);
  else
    favorites_.erase(it);

  Logger::log()->trace("GradientLibrary::set_favorite: '{}' -> {}", name, on);
  on_modified();
}

const std::vector<std::string> &GradientLibrary::favorites() const {
  return favorites_;
}

GradientSort GradientLibrary::sort() const { return sort_; }

void GradientLibrary::set_sort(GradientSort sort) {
  if (sort == sort_)
    return;
  sort_ = sort;
  on_modified();
}

nlohmann::json GradientLibrary::json_to() const {
  nlohmann::json json = gradient_file_json(presets_);
  json["favorites"] = favorites_;
  json["sort"] = std::string(to_string(sort_));
  return json;
}

bool GradientLibrary::json_from(const nlohmann::json &json) {
  if (!json.is_object() || !json.contains("gradients") ||
      !json["gradients"].is_array())
    return false;

  std::vector<Preset> presets =
      parse_preset_list(json["gradients"], kDefaultName);

  std::vector<std::string> favorites;
  if (json.contains("favorites") && json["favorites"].is_array())
    for (const auto &f : json["favorites"]) {
      if (!f.is_string())
        continue;
      const std::string name = f.get<std::string>();
      if (std::find(favorites.begin(), favorites.end(), name) ==
          favorites.end())
        favorites.push_back(name);
    }

  GradientSort sort = GradientSort::Default;
  if (json.contains("sort") && json["sort"].is_string())
    if (const auto s =
            gradient_sort_from_string(json["sort"].get<std::string>()))
      sort = *s;

  presets_.clear();
  for (auto &preset : presets) {
    preset.name = unique_name(preset.name);
    presets_.push_back(std::move(preset));
  }
  favorites_ = std::move(favorites);
  sort_ = sort;

  changed.notify();
  return true;
}

GradientImportReport
GradientLibrary::import_file(const std::filesystem::path &path) {
  GradientImportReport report;

  nlohmann::json json;
  if (!read_json_file(path, json))
    return report;

  const auto parsed = parse_gradient_file(json, path.stem().string());
  if (!parsed) {
    Logger::log()->warn("GradientLibrary::import_file: no gradients in '{}'",
                        path.string());
    return report;
  }

  for (Preset preset : *parsed) {
    if (const Preset *existing = find(preset.name)) {
      if (existing->stops == preset.stops) {
        ++report.skipped;
        continue;
      }
      preset.name = unique_name(preset.name);
      ++report.renamed;
    } else {
      ++report.added;
    }
    presets_.push_back(std::move(preset));
  }

  report.ok = true;

  Logger::log()->trace(
      "GradientLibrary::import_file: '{}': {} added, {} renamed, {} skipped",
      path.string(), report.added, report.renamed, report.skipped);

  if (report.added + report.renamed > 0)
    on_modified();
  return report;
}

bool GradientLibrary::export_file(const std::filesystem::path &path,
                                  const std::vector<Preset> &presets) const {
  Logger::log()->trace("GradientLibrary::export_file: {} presets to '{}'",
                       presets.size(), path.string());
  return write_json_file(path, gradient_file_json(presets));
}

void GradientLibrary::on_modified() {
  if (autosave_ && !path_.empty())
    save();
  changed.notify();
}

} // namespace meta
