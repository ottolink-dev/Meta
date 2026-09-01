/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include "meta_qt/container_widget.hpp"
#include "meta_qt/ui/theme.hpp"
#include "meta_qt/widgets/collapsible_section.hpp"

class QVariantAnimation;

namespace meta::qt::industrial
{

/** @brief Collapsible section that glides open and shut.
 *
 * The header is restyled through a stylesheet on the inherited QToolButton, so
 * this class only has to own the animation.
 *
 * Three things about animating a collapse in Qt, each of which looks like a
 * different bug when got wrong:
 *
 * - Animate setFixedHeight, not maximumHeight. Qt clamps maximumHeight upward
 *   against minimumHeight, so the body springs to full size for a frame and
 *   then snaps away.
 * - Disable the body's layout for the duration. A live layout reads a shrinking
 *   parent as a squeeze and redistributes the shortfall across every row, so
 *   the rows visibly compress instead of being clipped.
 * - Measure the expanded height while the body is actually laid out.
 *   sizeHint() on a hidden, never-laid-out widget overestimates, and animating
 *   to it overshoots and snaps back.
 */
class Section : public CollapsibleSection
{
  Q_OBJECT

public:
  Section(const QString &title, const Theme &theme, QWidget *parent = nullptr);

  void set_expanded(bool new_state) override;

private:
  int measured_body_height() const;

  const Theme       *theme_ = nullptr;
  QVariantAnimation *animation_ = nullptr;
  bool               first_apply_ = true;
};

/** @brief Section factory matching the industrial rows.
 *
 * `theme` must outlive every section the factory builds. Pass one owned by the
 * ThemeRegistry.
 */
SectionFactory make_section_factory(const Theme &theme);

} // namespace meta::qt::industrial
