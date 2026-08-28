/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <map>
#include <string>
#include <vector>

#include <QColor>
#include <QFont>

namespace meta::qt
{

/** @brief A monospaced font that exists on this platform.
 *
 * Naming a family directly is a portability trap: `Menlo` is macOS-only and
 * elsewhere silently degrades every numeric readout to a proportional face,
 * which looks like a layout bug rather than a missing font. The result is
 * resolved once and cached.
 */
QFont mono_font(int pixel_size);

/// The UI sans face. No family override -- the system font is correct here.
QFont ui_font(int pixel_size, bool bold = false, qreal letter_spacing = 0.0);

/** @brief Geometry and timing constants for a design.
 *
 * Separate from the palette because a colourway swap changes colours only,
 * while a design swap may change both.
 */
struct Metrics
{
  // --- rows
  int   row_height = 36;
  int   label_min_width = 90;
  int   label_max_width = 168;
  qreal label_width_ratio = 0.3;
  int   gap = 12;
  int   narrow_threshold = 430; ///< row width below which the narrow branch applies

  // --- value field
  int value_field_width = 74;
  int value_field_width_narrow = 64;
  int value_field_height = 24;

  // --- rail and thumb
  int rail_height = 6;
  int rail_radius = 1;
  int thumb_width = 10;
  int thumb_height = 18;

  // --- switch rows
  int check_row_height = 28;
  int switch_width = 36;
  int switch_height = 18;
  int knob_size = 12;
  int knob_inset = 3;

  // --- section
  int section_header_height = 38;
  int section_body_padding_x = 20;
  int section_body_padding_x_narrow = 12;
  int section_body_padding_y = 12;
  int section_row_spacing = 10;

  // --- shared
  int radius = 2;
  int glide_ms = 260;   ///< value glide; nothing snaps
  int switch_ms = 150;  ///< switch knob slide
  int section_ms = 200; ///< disclosure rotation
};

/** @brief A complete colourway plus geometry.
 *
 * A value type, deliberately: two panels may hold different themes, a theme is
 * trivially testable, and swapping one is an assignment rather than a mutation
 * of process-wide state. Controls receive a `const Theme &` that outlives them
 * (owned by the ThemeRegistry) and read it at paint time.
 *
 * Derived colours are exposed as *functions* rather than baked swatches, because
 * the formula is the thing that must survive an accent change.
 */
struct Theme
{
  std::string name = "industrial-dark";

  // --- surfaces
  QColor page{"#2b2b2b"};
  QColor bar{"#262626"};
  QColor section_header{"#333333"};
  QColor section_header_hover{"#383838"};
  QColor section_header_press{"#303030"};
  QColor rail_well{"#1c1c1c"};
  QColor field{"#1f1f1f"};
  QColor field_hover{"#262626"};
  QColor field_editing{"#161616"};
  QColor switch_track_off{"#1f1f1f"};

  // --- hairlines and bevels
  QColor bevel_top{"#3d3d3d"};
  QColor bevel_bottom{"#232323"};
  QColor hairline{"#1a1a1a"};
  QColor rail_well_border{"#161616"};
  QColor field_border{"#4a4a4a"};
  QColor field_border_hover{"#5a5a5a"};

  // --- ink. Only text encodes state; see state_ink().
  QColor ink_primary{"#e0e0e0"};
  QColor ink_section_title{"#d0d0d0"};
  QColor ink_secondary{"#9a9a9a"}; ///< value at default
  QColor ink_dim{"#8a8a8a"};
  QColor ink_locked{"#606060"};
  QColor ink_modified{"#ffffff"};
  QColor ink_icon{"#c9c9c9"};

  // --- metal
  QColor thumb_top{"#d6d6d6"};
  QColor thumb_bottom{"#a8a8a8"};
  QColor thumb_border{"#1a1a1a"};
  QColor thumb_grip{"#5f5f5f"};
  QColor knob_on_top{"#e8e8e8"};
  QColor knob_on_bottom{"#b8b8b8"};
  QColor knob_off_top{"#8a8a8a"};
  QColor knob_off_bottom{"#6a6a6a"};

  // --- accent
  QColor accent{"#e08a2e"};

  /// Per-group accents, keyed by attribute category. Falls back to `accent`.
  std::map<std::string, QColor> group_accents = {{"Erosion", QColor("#cfa143")},
                                                 {"Downcutting", QColor("#3aa899")},
                                                 {"Scale", QColor("#7d9cc0")},
                                                 {"Flow", QColor("#c06478")},
                                                 {"Selective", QColor("#a08bb8")},
                                                 {"Other", QColor("#9a9a9a")}};

  // --- derived-colour opacities. Port the formula, not the swatch.
  qreal rail_fill_alpha = 0.9;
  qreal locked_rail_fill_alpha = 0.3;
  qreal locked_thumb_alpha = 0.4;
  qreal switch_track_on_darker = 1.7;

  Metrics metrics;

  // --- derived colours

  /// Accent for an attribute category, falling back to the chrome accent.
  QColor group_accent(const std::string &category) const;

  /// Rail fill: always the group accent, never a state colour.
  QColor rail_fill(const std::string &category, bool locked = false) const;

  /// Switch track when on.
  QColor switch_track_on() const;

  /** @brief The one colour state is allowed to change.
   *
   * @param modified `|value - default| > 1e-4`, not "changed since opened".
   */
  QColor state_ink(bool modified, bool locked) const;
};

/** @brief Owns the built-in themes and any registered by a host.
 *
 * Themes are resolved once at construction time. Switching requires a restart:
 * controls cache brushes and pixmaps derived from the theme, and invalidating
 * those on every theme change would cost more than the feature is worth until a
 * second colourway actually exists.
 */
class ThemeRegistry
{
public:
  static ThemeRegistry &instance();

  /// Register a theme under `theme.name`, replacing any existing entry.
  void add(Theme theme);

  /** @brief Look up a theme by name.
   *
   * Returns the default theme when `name` is unknown, so a stale settings file
   * degrades to something usable rather than to an unpainted panel.
   */
  const Theme &get(const std::string &name) const;

  /// Names of every registered theme, for a settings UI.
  std::vector<std::string> names() const;

  /// The theme returned when a lookup misses.
  const Theme &fallback() const;

private:
  ThemeRegistry();

  std::map<std::string, Theme> themes_;
  std::string                  fallback_name_;
};

} // namespace meta::qt
