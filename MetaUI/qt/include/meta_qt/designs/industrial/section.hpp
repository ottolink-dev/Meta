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

/** @brief Fixed-height window onto a widget kept at its natural size.
 *
 * The body is given its full height regardless of how much of it is shown, and
 * this widget simply crops it, because a child is clipped to its parent.
 *
 * That is the whole point. Animating a collapse by constraining the body makes
 * the layout negotiate: a live layout treats the shortfall as a squeeze and
 * compresses the rows, a Fixed policy clamps a maximumHeight back up to the
 * size hint, and a fixed height has to land exactly on the final hint or it
 * snaps when released. None of that arithmetic happens here -- the reveal is
 * just a number, and the body never changes size at all.
 */
class ClipBox : public QWidget
{
public:
  explicit ClipBox(QWidget *parent = nullptr);

  /// Takes ownership of `body` as its only child.
  void set_body(QWidget *body);

  /// Show `px` of the body, measured from its top.
  void set_reveal(int px);

  /// Track the body's natural height instead of a fixed reveal.
  void follow_body();

  /// The body's natural height at the current width.
  int body_height() const;

  QSize sizeHint() const override;

protected:
  void resizeEvent(QResizeEvent *event) override;

  /// Watches the body for size-hint changes so the crop follows it.
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  QWidget *body_ = nullptr;
  int      reveal_ = 0;
  bool     follow_ = true;
};

/** @brief Collapsible section that glides open and shut.
 *
 * The header is restyled through a stylesheet on the inherited QToolButton, so
 * this class only has to own the animation.
 *
 * The collapse is animated by moving a ClipBox's reveal, never by constraining
 * the body. Every earlier attempt here tried to squeeze the body itself and
 * each failed differently: a fixed height snapped when released because it did
 * not land on the final size hint; a maximumHeight was clamped straight back up
 * by the body's own Fixed policy; and leaving the body Preferred let the parent
 * layout redistribute space across *every* section, so one animating section
 * visibly nudged all the others, worse the further down the column they sat.
 *
 * Cropping sidesteps all of it. The body is never resized, so there is no
 * negotiation to get wrong.
 */
class Section : public CollapsibleSection
{
  Q_OBJECT

public:
  Section(const QString &title, const Theme &theme, QWidget *parent = nullptr);

  void set_expanded(bool new_state) override;

protected:
  /// Draws the single card the header and rows sit on.
  void paintEvent(QPaintEvent *event) override;

private:
  const Theme       *theme_ = nullptr;
  ClipBox           *clip_ = nullptr;
  QVariantAnimation *animation_ = nullptr;
  bool               first_apply_ = true;

  /** @brief Where the animation is heading.
   *
   * Explicit rather than read back from the body's visibility, which stays true
   * throughout a collapse and would make the finish handler undo the collapse
   * it just performed.
   */
  bool expanded_ = true;
};

/** @brief Section factory matching the industrial rows.
 *
 * `theme` must outlive every section the factory builds. Pass one owned by the
 * ThemeRegistry.
 */
SectionFactory make_section_factory(const Theme &theme);

} // namespace meta::qt::industrial
