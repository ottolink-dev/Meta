/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <QString>
#include <algorithm>
#include <cmath>
#include <string>
namespace meta::qt
{
/** @brief True when a std::format spec asks for scientific or general form.
 *
 * Looks only at the presentation type, the last character before the closing
 * brace, so "{:.2e}" and "{:10.3E}" both match while "{:.2f}" does not. A
 * spec that names one of these is asking for an exponent, and the readout has
 * to give it one rather than substituting a fixed rendering of its own.
 */
inline bool has_exponent_format(const std::string &spec)
{
  const auto close = spec.find_last_of('}');
  if (close == std::string::npos || close == 0) return false;

  const char type = spec[close - 1];
  return type == 'e' || type == 'E' || type == 'g' || type == 'G';
}

// Ordinary values stay compact; small nonzero values must never read as zero.
inline QString display_float(float value, int decimals = 2)
{
  const double magnitude = std::abs(double(value));
  if (!std::isfinite(value)) return QString::number(value);
  decimals = std::clamp(decimals, 0, 8);
  if (magnitude == 0 || magnitude >= std::pow(10., -decimals))
    return QString::number(value == 0 ? 0 : value, 'f', decimals);
  if (magnitude < 1e-10) return QString::number(value, 'g', 6);
  const int precision = std::min(12,
                                 int(std::ceil(-std::log10(magnitude))) + 5);
  QString   text = QString::number(value, 'f', precision);
  while (text.endsWith('0') && text.size() - text.indexOf('.') - 1 > decimals)
    text.chop(1);
  return text;
}
} // namespace meta::qt
