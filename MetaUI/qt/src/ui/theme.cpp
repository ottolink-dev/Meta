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
      if (available.contains(candidate)) return candidate;

    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
  }();

  QFont font(family);
  font.setPixelSize(pixel_size);
  font.setStyleHint(QFont::Monospace);
  return font;
}

QFont ui_font(int pixel_size, bool bold, qreal letter_spacing)
{
  // Probed rather than left to the default. Qt's fallback is whatever the
  // platform hands out, which on some setups is a wide, soft face that reads
  // nothing like the rest of the chrome. Every candidate here is a neutral
  // grotesque; if none is installed we fall through to the platform default,
  // which is the current behaviour rather than a regression.
  static const QString family = []() -> QString
  {
    const QStringList candidates = {"Inter",
                                    "Roboto",
                                    "Segoe UI Variable Text",
                                    "Segoe UI",
                                    "Noto Sans",
                                    "DejaVu Sans"};

    const QStringList available = QFontDatabase::families();
    for (const QString &candidate : candidates)
      if (available.contains(candidate)) return candidate;

    return QString();
  }();

  QFont font;
  if (!family.isEmpty()) font.setFamily(family);
  font.setPixelSize(pixel_size);
  font.setBold(bold);
  if (letter_spacing != 0.0)
    font.setLetterSpacing(QFont::AbsoluteSpacing, letter_spacing);
  return font;
}

QFont row_label_font()
{
  // QApplication::font() rather than ui_font(): the point is to match whatever
  // the host is using, so the panel blends into the application instead of
  // announcing itself.
  QFont font = QApplication::font();

  // 13px rather than 12. At 12 the labels sat a step below the host's own
  // text and the panel read as small print; 13 matches the value field's mono
  // face, so a row's two halves are the same size.
  font.setPixelSize(13);

  // Medium rather than the default Normal. At this size a regular weight
  // leaves the stems thin enough that antialiasing does most of the work of
  // forming them, which reads as blurry rather than light. One step up puts
  // enough ink in the glyph to look deliberate, without promoting the row
  // label to the weight the section titles use.
  font.setWeight(QFont::Medium);

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
QColor lift(const QColor &c, qreal t)
{
  return mix(c, QColor(255, 255, 255), t);
}

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

  const bool is_dark = window.lightness() < 128;

  // --- surfaces
  t.page = window;
  t.bar = is_dark ? sink(window, 0.06) : lift(window, 0.06);

  // A clear step off the page. On dark themes we lift the card above the background;
  // on light themes we sink it (or lift towards pure white).
  t.section_surface = is_dark ? lift(window, 0.15) : sink(window, 0.06);

  // The header shares the card surface so a section reads as one block rather
  // than a bar with a differently coloured body under it. Hover and press are
  // small steps off that, not a different colour.
  t.section_header = t.section_surface;
  t.section_header_hover = is_dark ? lift(t.section_surface, 0.06)
                                   : sink(t.section_surface, 0.04);
  t.section_header_press = is_dark ? sink(t.section_surface, 0.04)
                                   : lift(t.section_surface, 0.04);
  t.rail_well = is_dark ? sink(window, 0.35) : lift(window, 0.35);
  t.field = base;
  t.field_hover = is_dark ? lift(base, 0.05) : sink(base, 0.04);
  t.field_editing = is_dark ? sink(base, 0.25) : lift(base, 0.15);
  t.switch_track_off = base;

  // --- hairlines and bevels
  t.bevel_top = is_dark ? lift(window, 0.12) : sink(window, 0.08);
  t.bevel_bottom = is_dark ? sink(window, 0.10) : lift(window, 0.08);
  t.hairline = is_dark ? sink(window, 0.40) : lift(window, 0.30);
  t.rail_well_border = is_dark ? sink(window, 0.50) : lift(window, 0.40);
  t.field_border = mid;
  t.field_border_hover = is_dark ? lift(mid, 0.15) : sink(mid, 0.15);

  // --- ink. Dimming blends towards the window, so it reads as "less
  // prominent" whichever side of the light/dark line the scheme sits on.
  //
  // Every blend is shallower than the first pass. Dimming towards the window
  // is the right formula, but it was applied hard enough that a row label and
  // its value both landed well short of the host's own text, which reads as
  // washed out rather than as a hierarchy. These keep the same ordering with
  // less distance between the steps.
  t.ink_primary = text;
  t.ink_section_title = mix(text, window, 0.05);
  t.ink_secondary = mix(text, window, 0.22);
  t.ink_dim = mix(text, window, 0.34);
  t.ink_icon = mix(text, window, 0.12);
  t.ink_locked = palette.color(QPalette::Disabled, QPalette::Text);

  // BrightText is the maximum-contrast ink, which is exactly what "modified"
  // wants. Some palettes leave it equal to Text, in which case push away from
  // the window instead so the modified state stays visibly distinct.
  const QColor bright = palette.color(QPalette::Active, QPalette::BrightText);
  t.ink_modified = bright == text
                       ? mix(text,
                             window.lightness() < 128 ? QColor(255, 255, 255)
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

QColor Theme::switch_track_on() const
{
  return accent.darker(int(switch_track_on_darker * 100));
}

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

void ThemeRegistry::update_palette_theme(const QPalette &palette)
{
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
