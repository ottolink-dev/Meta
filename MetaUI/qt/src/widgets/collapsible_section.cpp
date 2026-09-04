/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/widgets/collapsible_section.hpp"

namespace meta::qt
{

CollapsibleSection::CollapsibleSection(const QString &title, QWidget *parent)
    : QWidget(parent)
{
  auto *layout = new QVBoxLayout(this);

  layout->setAlignment(Qt::AlignTop);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(1);

  toggle_button = new QToolButton(this);
  toggle_button->setText(title);
  toggle_button->setCheckable(true);
  toggle_button->setChecked(true);
  toggle_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toggle_button->setArrowType(Qt::DownArrow);

  content = new QWidget(this);
  content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  content_layout = new QVBoxLayout(content);
  content_layout->setAlignment(Qt::AlignTop);

  layout->addWidget(toggle_button);
  layout->addWidget(content);

  connect(toggle_button,
          &QToolButton::toggled,
          this,
          [this](bool checked) { this->set_expanded(checked); });
}

bool CollapsibleSection::is_expanded() const { return content->isVisible(); }

void CollapsibleSection::set_expanded(bool new_state)
{
  content->setVisible(new_state);
  toggle_button->setArrowType(new_state ? Qt::DownArrow : Qt::RightArrow);
  QSignalBlocker blocker(toggle_button);
  toggle_button->setChecked(new_state);

  Q_EMIT expanded_state_changed(new_state);
}

} // namespace meta::qt
