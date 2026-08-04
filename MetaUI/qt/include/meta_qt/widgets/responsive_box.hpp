/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once

#include <algorithm>

#include <QBoxLayout>
#include <QResizeEvent>
#include <QWidget>

namespace meta::qt
{

// ---------------------------------------------------------------------------
// ResponsiveBox
//
// A tiny container widget that lays its children out horizontally when there
// is enough room, and stacks them vertically when the available width drops
// below the width needed to show them side-by-side.
//
// The reflow threshold is computed from the children's own minimum widths, so
// the box adapts automatically as the children (re)compute their geometry.
//
// Crucially, the box reports a minimum width of ONE child (the widest single
// child), NOT the horizontal sum — so its parent panel is free to narrow
// below the two-child width, which is precisely what triggers the switch to a
// stacked layout. To make that stick, the internal layout uses
// SetNoConstraint so it never forces the horizontal-sum minimum back onto the
// widget.
//
// No Q_OBJECT: this class introduces no new signals/slots, it only overrides
// protected virtuals, so it needs no moc.
// ---------------------------------------------------------------------------

class ResponsiveBox : public QWidget
{
public:
  explicit ResponsiveBox(QWidget *parent = nullptr) : QWidget(parent)
  {
    this->box = new QBoxLayout(QBoxLayout::LeftToRight, this);
    this->box->setContentsMargins(0, 0, 0, 0);
    this->box->setSpacing(this->box_spacing);
    // Do not let the layout push its (horizontal-sum) minimum onto us — we
    // want to be allowed to shrink to a single child's width and reflow.
    this->box->setSizeConstraint(QLayout::SetNoConstraint);
  }

  // Add a child to the responsive pair (stretch factor as for QBoxLayout).
  void add_widget(QWidget *w, int stretch = 0)
  {
    this->box->addWidget(w, stretch);
  }

  void set_spacing(int spacing)
  {
    this->box_spacing = spacing;
    this->box->setSpacing(spacing);
  }

  // Extra px of tolerance so the direction does not flip back and forth right
  // at the threshold.
  void set_hysteresis(int px) { this->hysteresis = px; }

  QSize minimumSizeHint() const override
  {
    // Width: allow collapsing to a single child (one stacked row); this is
    // what lets the enclosing panel narrow below the two-child width.
    QSize s = this->box->minimumSize(); // height reflects current direction
    s.setWidth(this->single_child_min_width());
    return s;
  }

protected:
  void resizeEvent(QResizeEvent *event) override
  {
    QWidget::resizeEvent(event);
    this->reflow(this->width());
  }

private:
  int single_child_min_width() const
  {
    int w = 0;
    for (int i = 0; i < this->box->count(); ++i)
      if (QWidget *cw = this->box->itemAt(i)->widget())
        w = std::max(w, this->child_min_width(cw));
    return w;
  }

  int side_by_side_min_width() const
  {
    int w = 0;
    int n = 0;
    for (int i = 0; i < this->box->count(); ++i)
      if (QWidget *cw = this->box->itemAt(i)->widget())
      {
        w += this->child_min_width(cw);
        ++n;
      }
    if (n > 1) w += (n - 1) * this->box_spacing;
    return w;
  }

  static int child_min_width(QWidget *cw)
  {
    int w = cw->minimumWidth();
    if (w <= 0) w = cw->minimumSizeHint().width();
    if (w <= 0) w = cw->sizeHint().width();
    return std::max(w, 0);
  }

  void reflow(int avail_width)
  {
    const int need = this->side_by_side_min_width();

    QBoxLayout::Direction want;
    if (this->box->direction() == QBoxLayout::LeftToRight)
      // Currently horizontal: stack once we can no longer fit the pair.
      want = (avail_width < need) ? QBoxLayout::TopToBottom
                                  : QBoxLayout::LeftToRight;
    else
      // Currently stacked: only go back horizontal with some slack, to avoid
      // flip-flopping right at the boundary.
      want = (avail_width >= need + this->hysteresis) ? QBoxLayout::LeftToRight
                                                      : QBoxLayout::TopToBottom;

    if (want != this->box->direction())
    {
      this->box->setDirection(want);
      // Height requirement changes between the two directions.
      this->updateGeometry();
    }
  }

  QBoxLayout *box = nullptr;
  int         box_spacing = 4;
  int         hysteresis = 16;
};

} // namespace meta::qt
