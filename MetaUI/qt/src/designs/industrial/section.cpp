/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/section.hpp"

#include <algorithm>

#include <QEasingCurve>
#include <QPainter>
#include <QResizeEvent>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariantAnimation>

#include "meta/logger.hpp"

namespace meta::qt::industrial
{

namespace
{

/// Object name the body background rule is selected by.
constexpr char kSectionBodyObjectName[] = "MetaIndustrialSectionBody";

/** @brief Stylesheet for the header, applied to the button itself.
 *
 * Deliberately *not* set on the section. A widget carrying a stylesheet has Qt
 * draw its background through the style machinery, which overrides anything
 * paintEvent puts down -- which is why two attempts at a card background
 * produced no visible change at all. With the section stylesheet-free, its
 * paintEvent is authoritative again and can draw the card.
 *
 * The stock header renders as a *checked* QToolButton, which most styles paint
 * in the platform highlight colour. That is why an unstyled panel shows blue
 * bars between grey rows.
 */
QString header_stylesheet(const Theme &theme)
{
  const Metrics &m = theme.metrics;

  // Transparent by default so the section's single painted card shows through.
  // Giving the header its own background makes it a second surface that has to
  // be butted against the body, and no amount of spacing tweaking makes two
  // surfaces meet cleanly -- there is always a seam or an overlap.
  //
  // Hover and press still tint, because those are states of the header alone.
  // Every state names an explicit background, and it is the same colour the
  // section paints its card with.
  //
  // Relying on "transparent" did not work: if Qt falls back to its own painting
  // for any state -- :checked in particular, which every expanded section is --
  // the header comes out a different shade to the card behind it. Stating the
  // colour outright in every state means there is nothing left to fall back to.
  return QString("QToolButton,"
                 "QToolButton:checked,"
                 "QToolButton:pressed,"
                 "QToolButton:focus {"
                 " background-color: %1;"
                 " border: none;"
                 " outline: none;"
                 " color: %2;"
                 " padding: 0px 12px;"
                 " min-height: %3px;"
                 " text-align: left;"
                 " font-weight: bold;"
                 "}"
                 "QToolButton:hover { background-color: %4; color: %5; }")
      .arg(theme.section_header.name())
      .arg(theme.ink_section_title.name())
      .arg(m.section_header_height)
      .arg(theme.section_header_hover.name())
      .arg(theme.ink_primary.name());
}

} // namespace

// --- ClipBox

ClipBox::ClipBox(QWidget *parent) : QWidget(parent)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void ClipBox::set_body(QWidget *body)
{
  body_ = body;
  if (!body_) return;

  body_->setParent(this);
  body_->move(0, 0);
  body_->show();

  // The body's height can change after it is first laid out -- a canvas that
  // derives its height from its width is the obvious case. Without this the
  // body stays frozen at whatever it measured first and gets cropped.
  body_->installEventFilter(this);

  updateGeometry();
}

bool ClipBox::eventFilter(QObject *watched, QEvent *event)
{
  if (watched == body_ && event->type() == QEvent::LayoutRequest)
  {
    body_->setGeometry(0, 0, width(), body_height());
    if (follow_) updateGeometry();
  }

  return QWidget::eventFilter(watched, event);
}

int ClipBox::body_height() const
{
  if (!body_) return 0;

  // sizeHint() rather than height(): the body is laid out at its natural size
  // and never resized, so the hint is what it actually occupies.
  const int hint = body_->sizeHint().height();
  return hint > 0 ? hint : body_->height();
}

void ClipBox::set_reveal(int px)
{
  follow_ = false;
  reveal_ = std::max(0, px);
  updateGeometry();
}

void ClipBox::follow_body()
{
  follow_ = true;
  updateGeometry();
}

QSize ClipBox::sizeHint() const
{
  const int w = body_ ? body_->sizeHint().width() : 0;
  return QSize(w, follow_ ? body_height() : reveal_);
}

void ClipBox::resizeEvent(QResizeEvent *event)
{
  // Keep the body at full height whatever this widget's height is. The crop is
  // free: a child is clipped to its parent's bounds.
  if (body_) body_->setGeometry(0, 0, width(), body_height());

  QWidget::resizeEvent(event);
}

// --- Section

Section::Section(const QString &title, const Theme &theme, QWidget *parent)
    : CollapsibleSection(title, parent), theme_(&theme)
{
  toggle_button->setStyleSheet(header_stylesheet(theme));

  // The header and the body each paint their own background, in the same colour
  // the section paints its card with. All three agree, so there is no seam to
  // line up.
  //
  // Painting only the card does not work: both children are opaque and cover
  // it. A magenta test card showed through as nothing but a three-pixel line in
  // the gap between them, which is exactly what the card is still useful for --
  // it fills that gap, and nothing else.
  content->setObjectName(QString::fromLatin1(kSectionBodyObjectName));
  content->setAttribute(Qt::WA_StyledBackground, true);
  content->setStyleSheet(
      QString("#%1 { background-color: %2; border-bottom-left-radius: %3px;"
              " border-bottom-right-radius: %3px; }"
              // Stock widgets bring their own labels, and some fill a
              // background from the palette, which shows as a pale strip.
              "#%1 QLabel { background: transparent; }")
          .arg(QString::fromLatin1(kSectionBodyObjectName))
          .arg(theme.section_surface.name())
          .arg(theme.metrics.section_card_radius));

  // Without this the header is only as wide as its text, which reads as a
  // floating pill rather than a section bar. QToolButton defaults to a
  // non-expanding horizontal policy.
  toggle_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  toggle_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

  // Fixed vertically: the base class leaves sections Preferred, which lets the
  // panel's QVBoxLayout hand each one a share of the leftover space and
  // re-divide it whenever any section changes height. The trailing stretch in
  // the panel layout is where slack should go instead.
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  if (content_layout)
  {
    const Metrics &m = theme.metrics;
    content_layout->setContentsMargins(m.section_body_padding_x,
                                       m.section_body_padding_y,
                                       m.section_body_padding_x,
                                       m.section_body_padding_y);
    content_layout->setSpacing(m.section_row_spacing);
  }

  // Slot the body into a clip box so the animation never touches its size.
  if (auto *outer = qobject_cast<QVBoxLayout *>(layout()))
  {
    const Metrics &m = theme.metrics;
    // Matches the card inset in paintEvent, so the header and rows sit inside
    // the card rather than straddling its edge.
    outer->setContentsMargins(m.section_card_margin,
                              m.section_card_gap / 2,
                              m.section_card_margin,
                              m.section_card_gap / 2);

    outer->setSpacing(0);

    outer->removeWidget(content);

    clip_ = new ClipBox(this);
    clip_->setAutoFillBackground(false);
    clip_->setAttribute(Qt::WA_StyledBackground, false);
    clip_->set_body(content);
    outer->addWidget(clip_);
  }

  animation_ = new QVariantAnimation(this);
  animation_->setDuration(theme.metrics.section_ms);
  animation_->setEasingCurve(QEasingCurve::OutCubic);

  connect(animation_,
          &QVariantAnimation::valueChanged,
          this,
          [this](const QVariant &v) { clip_->set_reveal(v.toInt()); });

  connect(animation_,
          &QVariantAnimation::finished,
          this,
          [this]()
          {
            // Once open, track the body so later content changes are picked up
            // rather than being frozen at whatever the animation ended on.
            if (expanded_) clip_->follow_body();
          });
}

void Section::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const Metrics &m = theme_->metrics;

  // One card for the whole section, inset vertically by half the gap at each
  // end so consecutive sections are separated by unpainted page.
  const QRect card = rect().adjusted(m.section_card_margin,
                                     m.section_card_gap / 2,
                                     -m.section_card_margin,
                                     -m.section_card_gap / 2);

  painter.setPen(Qt::NoPen);
  painter.setBrush(theme_->section_surface);
  painter.drawRoundedRect(card, m.section_card_radius, m.section_card_radius);
}

void Section::set_expanded(bool new_state)
{
  const bool was_expanded = expanded_;
  expanded_ = new_state;

  toggle_button->setArrowType(new_state ? Qt::DownArrow : Qt::RightArrow);
  {
    QSignalBlocker blocker(toggle_button);
    toggle_button->setChecked(new_state);
  }

  if (!clip_)
  {
    content->setVisible(new_state);
    Q_EMIT expanded_state_changed(new_state);
    return;
  }

  // The first call restores persisted state during construction, before
  // anything is on screen. Animating that would play every section open at
  // startup, so seat it directly.
  if (first_apply_ || was_expanded == new_state)
  {
    first_apply_ = false;
    animation_->stop();

    if (new_state)
      clip_->follow_body();
    else
      clip_->set_reveal(0);

    update();
    Q_EMIT expanded_state_changed(new_state);
    return;
  }

  const int full = clip_->body_height();

  animation_->stop(); // a running animation ignores a retargeted end value
  animation_->setStartValue(new_state ? 0 : full);
  animation_->setEndValue(new_state ? full : 0);
  animation_->start();

  update();
  Q_EMIT expanded_state_changed(new_state);
}

SectionFactory make_section_factory(const Theme &theme)
{
  return [&theme](const QString &title) -> CollapsibleSection *
  { return new Section(title, theme); };
}

} // namespace meta::qt::industrial
