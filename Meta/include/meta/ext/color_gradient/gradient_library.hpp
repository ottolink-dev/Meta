/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "meta/core/event.hpp"
#include "meta/ext/color_gradient/color_gradient.hpp"

namespace meta {

/// Ordering applied to preset grids. Favourites are always pinned first.
enum class GradientSort {
  Default,   ///< host order, then library insertion order
  Name,      ///< case-insensitive name
  Luminance, ///< dark to light (gradient_luminance)
  Hue        ///< around the colour wheel, achromatic last (gradient_hue)
};

/// Persisted identifier of a sort key ("default", "name", "luminance", "hue").
std::string_view to_string(GradientSort sort);

/// Inverse of to_string(GradientSort); std::nullopt for unknown identifiers.
std::optional<GradientSort> gradient_sort_from_string(std::string_view text);

/// Outcome of GradientLibrary::import_file().
struct GradientImportReport {
  std::size_t added = 0;   ///< stored under their own name
  std::size_t renamed = 0; ///< stored under a suffixed name (name clash)
  std::size_t skipped = 0; ///< identical to an existing preset
  bool ok = false;         ///< file could be read and held gradients
};

/**
 * @brief Gradient file document ("meta.gradients", version 1) for a list of
 * presets:
 * @code
 * {"format": "meta.gradients", "version": 1,
 *  "gradients": [{"name": "...", "stops": [{"position": t, "color":
 * [r,g,b,a]}]}]}
 * @endcode
 */
nlohmann::json gradient_file_json(const std::vector<Preset> &presets);

/**
 * @brief Tolerant reader for gradient files.
 *
 * Accepts the document produced by gradient_file_json(), a bare array of
 * gradients, or a single gradient object. A gradient's stops may live under
 * "stops" or "value" (the ColorGradient::json_to() shape); a missing name
 * falls back to `fallback_name` (numbered when the file holds several);
 * colours with any component above 1 are read as 0-255; RGB colours get
 * alpha 1; entries with fewer than two valid stops are dropped.
 *
 * @return The parsed presets, or std::nullopt when nothing usable was found.
 */
std::optional<std::vector<Preset>>
parse_gradient_file(const nlohmann::json &json,
                    std::string_view fallback_name = "Gradient");

/**
 * @brief The user's gradient presets, shared by every gradient widget in the
 * process and persisted across projects.
 *
 * Holds user presets (insertion ordered), favourite names (which may also
 * refer to host presets that are not in the library) and the preferred sort
 * key. Every effective mutation autosaves to path() (when set) and notifies
 * `changed`. Hosts normally use instance(); the class stays instantiable for
 * tests and for hosts that want several libraries.
 *
 * The Qt GradientPicker assigns a default per-user path on first use when
 * none is set; call set_path() + load() beforehand to choose another.
 */
class GradientLibrary {
public:
  /// Fired after every effective mutation and after a successful load.
  Event<> changed;

  GradientLibrary() = default;

  /// Process-wide default library.
  static GradientLibrary &instance();

  // --- storage

  /// Sets the file used by load()/save() and by autosave. Does not load.
  void set_path(std::filesystem::path path);

  /// File used for persistence; empty when the library is in-memory only.
  const std::filesystem::path &path() const;

  /// Whether mutations save automatically (default true).
  void set_autosave(bool on);
  bool autosave() const;

  /**
   * @brief Replaces the content with the document at path().
   * @return false when no path is set, the file is missing or invalid; the
   * current content is left untouched in every failure case.
   */
  bool load();

  /// Writes json_to() to path(), creating parent directories.
  bool save() const;

  // --- user presets

  const std::vector<Preset> &presets() const;
  bool has(std::string_view name) const;
  const Preset *find(std::string_view name) const;

  /**
   * @brief Stores a preset. The name is trimmed ("Gradient" when empty) and
   * made unique with unique_name(); stops are sorted by position.
   * @return The name the preset was stored under.
   */
  std::string add(Preset preset);

  /// Replaces the stops of an existing preset (sorted by position).
  bool update(std::string_view name, std::vector<Stop> stops);

  /// Renames a preset, carrying its favourite flag along. Fails when `from`
  /// is unknown, `to` is blank, or `to` already exists (unless equal).
  bool rename(std::string_view from, std::string_view to);

  /// Removes a preset and its favourite flag.
  bool remove(std::string_view name);

  /// Removes every preset and favourite.
  void clear();

  /// `base` if free, otherwise "base (2)", "base (3)", ...; names in
  /// `reserved` (e.g. host presets) count as taken.
  std::string unique_name(std::string_view base,
                          const std::vector<std::string> &reserved = {}) const;

  // --- favourites

  bool is_favorite(std::string_view name) const;
  void set_favorite(std::string_view name, bool on);
  const std::vector<std::string> &favorites() const;

  // --- sort preference

  GradientSort sort() const;
  void set_sort(GradientSort sort);

  // --- serialization

  /// Library document: gradient_file_json() plus "favorites" and "sort".
  nlohmann::json json_to() const;

  /// Replaces the content from a library document; false (state untouched)
  /// unless `json` is an object with a "gradients" array.
  bool json_from(const nlohmann::json &json);

  /**
   * @brief Merges the gradients of a file into the library.
   *
   * Identical duplicates (same name and stops) are skipped; name clashes are
   * stored under unique_name(); everything else is added as is.
   */
  GradientImportReport import_file(const std::filesystem::path &path);

  /// Writes `presets` as a gradient file (no favourites, no sort).
  bool export_file(const std::filesystem::path &path,
                   const std::vector<Preset> &presets) const;

private:
  void on_modified();

  std::vector<Preset> presets_;
  std::vector<std::string> favorites_;
  GradientSort sort_ = GradientSort::Default;
  std::filesystem::path path_;
  bool autosave_ = true;
};

} // namespace meta
