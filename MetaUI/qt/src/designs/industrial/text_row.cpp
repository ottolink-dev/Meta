/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/designs/industrial/text_row.hpp"

#include <algorithm>

#include <QLineEdit>
#include <QPainter>
#include <QResizeEvent>

#include "meta_qt/designs/industrial/slider_chrome.hpp"

namespace meta::qt::industrial
{

TextRow::TextRow(Attribute<std::string> &attr,
                 const RowContext       &ctx,
                 QWidget                *parent)
    : Control<std::string>(ctx, parent)
{
  label_ = meta::common::label(attr);
  value_ = attr.value();
  read_only_ = meta::common::try_get<bool>(attr,
                                           meta::keys::ui::read_only,
                                           false) ||
               meta::common::widget_type(attr) == "ReadOnlyText";

  setFixedHeight(theme().metrics.row_height);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  field_ = new QLineEdit(this);
  field_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  field_->setFrame(false);

  // The host font rather than the mono face. A readout is a number, where
  // aligned digits matter; this is prose, and prose in a mono face reads as a
  // code editor.
  field_->setFont(row_label_font());
  field_->installEventFilter(this);

  refresh_field();
  restyle_field();

  connect(field_,
          &QLineEdit::editingFinished,
          this,
          [this]()
          {
            const std::string typed = field_->text().toStdString();
            if (typed == value_) return;

            begin_edit();
            value_ = typed;
            notify_value_changed();
            end_edit();
          });

  connect(field_,
          &QLineEdit::textEdited,
          this,
          [this]() { restyle_field(true); });
}

bool TextRow::can_render(const Attribute<std::string> &) { return true; }

void TextRow::set(const std::string &value)
{
  value_ = value;
  refresh_field();
  update();
}

QSize TextRow::sizeHint() const
{
  return QSize(theme().metrics.label_min_width + 160,
               theme().metrics.row_height);
}

// --- geometry

QRect TextRow::label_rect() const
{
  const Metrics &m = theme().metrics;

  // Same formula the sliders use, so labels line up down the whole panel
  // rather than every row type picking its own column.
  const int label_width = int(std::clamp<qreal>(width() * m.label_width_ratio,
                                                m.label_min_width,
                                                m.label_max_width));

  return QRect(0, 0, label_width, height());
}

QRect TextRow::field_rect() const
{
  const Metrics &m = theme().metrics;
  const int      x = label_rect().width() + m.gap;

  return QRect(x,
               (height() - m.value_field_height) / 2,
               std::max(0, width() - x),
               m.value_field_height);
}

// --- painting

void TextRow::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  QFont label_font = row_label_font();
  painter.setFont(label_font);
  painter.setPen(theme().state_ink(is_modified(), is_locked() || read_only_));
  painter.drawText(label_rect(),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   elide_label(QString::fromStdString(label_),
                               label_font,
                               label_rect().width()));
}

void TextRow::resizeEvent(QResizeEvent *event)
{
  field_->setGeometry(field_rect());
  QWidget::resizeEvent(event);
}

// --- state

void TextRow::on_state_changed()
{
  restyle_field(field_ && field_->hasFocus());
  update();
}

bool TextRow::eventFilter(QObject *watched, QEvent *event)
{
  if (watched == field_ &&
      (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut))
  {
    const bool editing = event->type() == QEvent::FocusIn;
    if (!editing) refresh_field();
    restyle_field(editing);
  }

  return Control<std::string>::eventFilter(watched, event);
}

// --- helpers

void TextRow::refresh_field()
{
  if (!field_ || field_->hasFocus()) return; // never overwrite mid-typing

  const QSignalBlocker blocker(field_);
  field_->setText(QString::fromStdString(value_));
}

void TextRow::restyle_field(bool editing)
{
  if (!field_) return;

  const bool locked = is_locked() || read_only_;

  field_->setReadOnly(locked);
  field_->setStyleSheet(
      field_stylesheet(theme(), editing && !locked, is_modified(), locked));
}

} // namespace meta::qt::industrial
