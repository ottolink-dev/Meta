/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/ui/control.hpp"

#include <QFontMetrics>
#include <QWheelEvent>

namespace meta::qt
{

ControlBase::ControlBase(const RowContext &ctx, QWidget *parent)
    : QWidget(parent),
      ctx_(ctx),
      theme_(ctx.theme ? ctx.theme : &ThemeRegistry::instance().fallback())
{
  setFocusPolicy(Qt::StrongFocus);
}

void ControlBase::set_locked(bool locked)
{
  if (locked_ == locked) return;
  locked_ = locked;
  on_state_changed();
}

void ControlBase::set_modified(bool modified)
{
  if (modified_ == modified) return;
  modified_ = modified;
  on_state_changed();
}

void ControlBase::begin_edit()
{
  if (editing_) return;
  editing_ = true;
  Q_EMIT edit_started();
}

void ControlBase::end_edit()
{
  if (!editing_) return;
  editing_ = false;
  Q_EMIT edit_ended();
}

void ControlBase::notify_value_changed() { Q_EMIT value_changed(); }

QString ControlBase::elide_label(const QString &text,
                                 const QFont   &font,
                                 int            width)
{
  const QFontMetrics metrics(font);
  const QString      elided = metrics.elidedText(text, Qt::ElideRight, width);

  if (elided != text && toolTip().isEmpty())
  {
    // The container sets ui.tooltip on the host row, not on the control, and it
    // does so before the first paint -- so an empty parent tooltip here really
    // does mean there is nothing to shadow.
    QWidget *host = parentWidget();
    if (!host || host->toolTip().isEmpty()) setToolTip(text);
  }

  return elided;
}

void ControlBase::handle_wheel(QWheelEvent *event) { event->ignore(); }

void ControlBase::wheelEvent(QWheelEvent *event)
{
  // Unfocused controls must let the wheel through to the scroll area, or
  // scrolling the panel edits whatever value happens to be under the cursor.
  if (!hasFocus() || locked_)
  {
    event->ignore();
    return;
  }

  handle_wheel(event);
}

} // namespace meta::qt
