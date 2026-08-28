/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/ui/theme.hpp"

#include <QFontDatabase>

namespace meta::qt
{

// --- fonts

QFont mono_font(int pixel_size)
{
  static const QString family = []() -> QString
  {
    // Probe rather than name a family: the obvious choices are each present on
    // exactly one platform, and a missing family falls back to a proportional
    // face without warning.
    const QStringList candidates = {"Consolas",
                                    "DejaVu Sans Mono",
                                    "Menlo",
                                    "Liberation Mono",
                                    "Noto Sans Mono",
                                    "Courier New"};

    const QStringList available = QFontDatabase::families();
    for (const QString &candidate : candidates)
      if (available.contains(candidate))
        return candidate;

    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
  }();

  QFont font(family);
  font.setPixelSize(pixel_size);
  font.setStyleHint(QFont::Monospace);
  return font;
}

QFont ui_font(int pixel_size, bool bold, qreal letter_spacing)
{
  QFont font;
  font.setPixelSize(pixel_size);
  font.setBold(bold);
  if (letter_spacing != 0.0)
    font.setLetterSpacing(QFont::AbsoluteSpacing, letter_spacing);
  return font;
}

// --- Theme

QColor Theme::group_accent(const std::string &category) const
{
  auto it = group_accents.find(category);
  return it == group_accents.end() ? accent : it->second;
}

QColor Theme::rail_fill(const std::string &category, bool locked) const
{
  QColor c = group_accent(category);
  c.setAlphaF(locked ? locked_rail_fill_alpha : rail_fill_alpha);
  return c;
}

QColor Theme::switch_track_on() const { return accent.darker(int(switch_track_on_darker * 100)); }

QColor Theme::state_ink(bool modified, bool locked) const
{
  if (locked) return ink_locked;
  return modified ? ink_modified : ink_secondary;
}

// --- ThemeRegistry

namespace
{

/// The reference colourway, sampled from a render rather than copied from notes.
Theme make_industrial_dark()
{
  Theme t;
  t.name = "industrial-dark";
  return t; // the struct defaults *are* industrial-dark
}

/** @brief A light colourway over the same geometry.
 *
 * Present so the theme layer has more than one occupant from day one -- a
 * mechanism with a single implementation is untested by construction, and this
 * is what proves the palette is genuinely swappable rather than nominally so.
 */
Theme make_industrial_light()
{
  Theme t;
  t.name = "industrial-light";

  t.page = QColor("#d9d9d9");
  t.bar = QColor("#cfcfcf");
  t.section_header = QColor("#c4c4c4");
  t.section_header_hover = QColor("#bcbcbc");
  t.section_header_press = QColor("#c9c9c9");
  t.rail_well = QColor("#b8b8b8");
  t.field = QColor("#ececec");
  t.field_hover = QColor("#e2e2e2");
  t.field_editing = QColor("#ffffff");
  t.switch_track_off = QColor("#b8b8b8");

  t.bevel_top = QColor("#e8e8e8");
  t.bevel_bottom = QColor("#b0b0b0");
  t.hairline = QColor("#a8a8a8");
  t.rail_well_border = QColor("#9e9e9e");
  t.field_border = QColor("#8a8a8a");
  t.field_border_hover = QColor("#6e6e6e");

  t.ink_primary = QColor("#1f1f1f");
  t.ink_section_title = QColor("#2b2b2b");
  t.ink_secondary = QColor("#5a5a5a");
  t.ink_dim = QColor("#6e6e6e");
  t.ink_locked = QColor("#a0a0a0");
  t.ink_modified = QColor("#000000");
  t.ink_icon = QColor("#3a3a3a");

  t.thumb_top = QColor("#fbfbfb");
  t.thumb_bottom = QColor("#d0d0d0");
  t.thumb_border = QColor("#8a8a8a");
  t.thumb_grip = QColor("#a8a8a8");
  t.knob_on_top = QColor("#ffffff");
  t.knob_on_bottom = QColor("#e0e0e0");
  t.knob_off_top = QColor("#f0f0f0");
  t.knob_off_bottom = QColor("#d4d4d4");

  return t;
}

} // namespace

ThemeRegistry::ThemeRegistry()
{
  add(make_industrial_dark());
  add(make_industrial_light());
  fallback_name_ = "industrial-dark";
}

ThemeRegistry &ThemeRegistry::instance()
{
  static ThemeRegistry registry;
  return registry;
}

void ThemeRegistry::add(Theme theme)
{
  const std::string key = theme.name;
  themes_[key] = std::move(theme);
}

const Theme &ThemeRegistry::get(const std::string &name) const
{
  auto it = themes_.find(name);
  return it == themes_.end() ? fallback() : it->second;
}

const Theme &ThemeRegistry::fallback() const
{
  return themes_.at(fallback_name_);
}

std::vector<std::string> ThemeRegistry::names() const
{
  std::vector<std::string> out;
  out.reserve(themes_.size());
  for (const auto &[name, _] : themes_)
    out.push_back(name);
  return out;
}

} // namespace meta::qt
