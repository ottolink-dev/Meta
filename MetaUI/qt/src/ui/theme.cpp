/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/ui/theme.hpp"

#include <QApplication>
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

namespace
{

/// Linear blend in RGB. `t` = 0 keeps `a`, `t` = 1 gives `b`.
QColor mix(const QColor &a, const QColor &b, qreal t)
{
  return QColor::fromRgbF(a.redF() * (1.0 - t) + b.redF() * t,
                          a.greenF() * (1.0 - t) + b.greenF() * t,
                          a.blueF() * (1.0 - t) + b.blueF() * t);
}

/// Push a surface down (recessed). Same direction on light and dark schemes.
QColor sink(const QColor &c, qreal t) { return mix(c, QColor(0, 0, 0), t); }

/// Push a surface up (raised). Same direction on light and dark schemes.
QColor lift(const QColor &c, qreal t) { return mix(c, QColor(255, 255, 255), t); }

} // namespace

Theme Theme::from_palette(const QPalette &palette, const std::string &name)
{
  Theme t;
  t.name = name;

  const QColor window = palette.color(QPalette::Active, QPalette::Window);
  const QColor base = palette.color(QPalette::Active, QPalette::Base);
  const QColor text = palette.color(QPalette::Active, QPalette::Text);
  const QColor mid = palette.color(QPalette::Active, QPalette::Mid);
  const QColor light = palette.color(QPalette::Active, QPalette::Light);

  // --- surfaces
  t.page = window;
  t.bar = sink(window, 0.06);
  t.section_header = lift(window, 0.06);
  t.section_header_hover = lift(window, 0.10);
  t.section_header_press = lift(window, 0.03);
  t.rail_well = sink(window, 0.35);
  t.field = base;
  t.field_hover = lift(base, 0.05);
  t.field_editing = sink(base, 0.25);
  t.switch_track_off = base;

  // --- hairlines and bevels
  t.bevel_top = lift(window, 0.12);
  t.bevel_bottom = sink(window, 0.10);
  t.hairline = sink(window, 0.40);
  t.rail_well_border = sink(window, 0.50);
  t.field_border = mid;
  t.field_border_hover = lift(mid, 0.15);

  // --- ink. Dimming blends towards the window, so it reads as "less
  // prominent" whichever side of the light/dark line the scheme sits on.
  t.ink_primary = text;
  t.ink_section_title = mix(text, window, 0.12);
  t.ink_secondary = mix(text, window, 0.38);
  t.ink_dim = mix(text, window, 0.48);
  t.ink_icon = mix(text, window, 0.20);
  t.ink_locked = palette.color(QPalette::Disabled, QPalette::Text);

  // BrightText is the maximum-contrast ink, which is exactly what "modified"
  // wants. Some palettes leave it equal to Text, in which case push away from
  // the window instead so the modified state stays visibly distinct.
  const QColor bright = palette.color(QPalette::Active, QPalette::BrightText);
  t.ink_modified = bright == text ? mix(text, window.lightness() < 128
                                                  ? QColor(255, 255, 255)
                                                  : QColor(0, 0, 0),
                                        0.45)
                                  : bright;

  // --- metal
  t.thumb_top = light;
  t.thumb_bottom = sink(light, 0.22);
  t.thumb_border = sink(window, 0.55);
  t.thumb_grip = mid;
  t.knob_on_top = light;
  t.knob_on_bottom = sink(light, 0.18);
  t.knob_off_top = mid;
  t.knob_off_bottom = sink(mid, 0.18);

  // --- accent. Follow the host rather than imposing one; assign over this
  // afterwards to pin a specific accent.
  t.accent = palette.color(QPalette::Active, QPalette::Highlight);

  return t;
}

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

/** @brief The reference colourway, sampled from a render rather than notes.
 *
 * Kept as an explicit option so the design can be seen exactly as it was
 * drawn, independent of whatever palette the host happens to run. The *default*
 * is the palette-derived theme, not this.
 */
Theme make_industrial_dark()
{
  Theme t;
  t.name = "industrial-dark";
  return t; // the struct's member initialisers are industrial-dark
}

} // namespace

ThemeRegistry::ThemeRegistry()
{
  add(make_industrial_dark());
  fallback_name_ = kPaletteTheme;
}

void ThemeRegistry::ensure_palette_theme() const
{
  if (themes_.find(kPaletteTheme) != themes_.end()) return;

  const QPalette palette = QApplication::palette();
  themes_[kPaletteTheme] = Theme::from_palette(palette, kPaletteTheme);
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
  ensure_palette_theme();

  auto it = themes_.find(name);
  return it == themes_.end() ? fallback() : it->second;
}

const Theme &ThemeRegistry::fallback() const
{
  ensure_palette_theme();
  return themes_.at(fallback_name_);
}

std::vector<std::string> ThemeRegistry::names() const
{
  ensure_palette_theme();

  std::vector<std::string> out;
  out.reserve(themes_.size());
  for (const auto &[name, _] : themes_)
    out.push_back(name);
  return out;
}

} // namespace meta::qt
