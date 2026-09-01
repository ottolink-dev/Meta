/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/section.hpp"

#include <QEasingCurve>
#include <QToolButton>
#include <QVariantAnimation>

namespace meta::qt::industrial
{

namespace
{

QString header_stylesheet(const Theme &theme)
{
  const Metrics &m = theme.metrics;

  // The header is the inherited QToolButton. Styling it by type selector keeps
  // this independent of how CollapsibleSection lays itself out.
  //
  // The stock header renders as a *checked* QToolButton, which most styles
  // paint in the platform highlight colour. That is why an unstyled panel shows
  // blue bars between grey rows.
  return QString("QToolButton {"
                 " background: %1;"
                 " color: %2;"
                 " border: none;"
                 " border-top: 1px solid %3;"
                 " border-bottom: 1px solid %4;"
                 " padding: 0px 12px;"
                 " min-height: %5px;"
                 " text-align: left;"
                 " font-weight: bold;"
                 " letter-spacing: 1px;"
                 "}"
                 "QToolButton:hover { background: %6; }"
                 "QToolButton:pressed { background: %7; }"
                 "QToolButton:checked { background: %1; color: %2; }")
      .arg(theme.section_header.name())
      .arg(theme.ink_section_title.name())
      .arg(theme.bevel_top.name())
      .arg(theme.bevel_bottom.name())
      .arg(m.section_header_height)
      .arg(theme.section_header_hover.name())
      .arg(theme.section_header_press.name());
}

} // namespace

Section::Section(const QString &title, const Theme &theme, QWidget *parent)
    : CollapsibleSection(title, parent), theme_(&theme)
{
  setStyleSheet(header_stylesheet(theme));

  if (content_layout)
  {
    const Metrics &m = theme.metrics;
    content_layout->setContentsMargins(m.section_body_padding_x,
                                       m.section_body_padding_y,
                                       m.section_body_padding_x,
                                       m.section_body_padding_y);
    content_layout->setSpacing(m.section_row_spacing);
  }

  animation_ = new QVariantAnimation(this);
  animation_->setDuration(theme.metrics.section_ms);
  animation_->setEasingCurve(QEasingCurve::OutCubic);

  connect(animation_,
          &QVariantAnimation::valueChanged,
          this,
          [this](const QVariant &v) { content->setFixedHeight(v.toInt()); });

  connect(animation_,
          &QVariantAnimation::finished,
          this,
          [this]()
          {
            // Hand the body back to the layout system once it has settled,
            // otherwise it stays pinned at the animated height and stops
            // responding to content changes.
            if (content->layout()) content->layout()->setEnabled(true);

            if (is_expanded())
            {
              content->setMinimumHeight(0);
              content->setMaximumHeight(QWIDGETSIZE_MAX);
            }
            else
            {
              content->setVisible(false);
            }
          });
}

int Section::measured_body_height() const
{
  // Measure the real laid-out height where possible. sizeHint() on a hidden,
  // never-laid-out widget overestimates, and animating to it overshoots.
  if (content->isVisible() && content->height() > 0) return content->height();
  return content->sizeHint().height();
}

void Section::set_expanded(bool new_state)
{
  const bool was_expanded = is_expanded();

  toggle_button->setArrowType(new_state ? Qt::DownArrow : Qt::RightArrow);
  {
    QSignalBlocker blocker(toggle_button);
    toggle_button->setChecked(new_state);
  }

  // The first call comes from restoring persisted state during construction,
  // before anything is on screen. Animating that would play every section open
  // on startup, so seat it directly.
  if (first_apply_ || was_expanded == new_state)
  {
    first_apply_ = false;
    content->setVisible(new_state);
    Q_EMIT expanded_state_changed(new_state);
    return;
  }

  const int target = new_state ? measured_body_height() : 0;
  const int start = new_state ? 0 : measured_body_height();

  // A live layout treats a shrinking parent as a squeeze and redistributes the
  // shortfall across the rows, so they compress instead of being clipped.
  if (content->layout()) content->layout()->setEnabled(false);

  content->setVisible(true);
  content->setFixedHeight(start);

  animation_->stop(); // a running animation ignores a retargeted end value
  animation_->setStartValue(start);
  animation_->setEndValue(target);
  animation_->start();

  Q_EMIT expanded_state_changed(new_state);
}

SectionFactory make_section_factory(const Theme &theme)
{
  return [&theme](const QString &title) -> CollapsibleSection *
  { return new Section(title, theme); };
}

} // namespace meta::qt::industrial
