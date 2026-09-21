/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#ifdef META_ENABLE_GLM_TYPES
#include "meta_qt/designs/industrial/linked_sliders.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
namespace meta::qt::industrial
{
LinkedSliders::LinkedSliders(Attribute<glm::vec2> &attr,
                             const RowContext     &ctx,
                             QWidget              *parent)
    : Control<glm::vec2>(ctx, parent),
      axes_theme_(theme()),
      state_(&attr.state())
{
  if (auto *flag = state_->try_value<bool>(meta::keys::state::locked_xy))
    linked_ = *flag;
  axes_theme_.metrics.label_min_width = 18;
  axes_theme_.metrics.label_max_width = 24;
  axes_theme_.metrics.label_width_ratio = .06;
  axes_theme_.ink_locked = theme().ink_secondary;
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(2);
  auto *heading = new QHBoxLayout;
  auto *label = new QLabel(QString::fromStdString(meta::common::label(attr)));
  label->setFont(row_label_font());
  label->setStyleSheet(QString("color: %1; background: transparent;")
                           .arg(theme().ink_primary.name()));
  heading->addWidget(label, 1);
  link_ = new QToolButton;
  link_->setCheckable(true);
  link_->setChecked(linked_);
  link_->setFixedSize(58, 24);
  link_->setFont(ui_font(11, true));
  link_->setAccessibleName("Link X and Y");
  link_->setToolTip("Link axes: changes to X also update Y");
  link_->setCursor(Qt::PointingHandCursor);
  link_->setStyleSheet(
      QString("QToolButton { color: %1; background: %2; border: 1px solid %3; "
              "border-radius: 5px; } QToolButton:checked { border-color: %4; "
              "background: "
              "%5; } QToolButton:hover { border-color: %4; }")
          .arg(theme().ink_primary.name(),
               theme().field.name(),
               theme().field_border.name(),
               theme().accent.name(),
               theme().section_header_hover.name()));
  heading->addWidget(link_);
  layout->addLayout(heading);
  for (int i = 0; i < 2; ++i)
  {
    auto &metadata = axes_[i].metadata();
    metadata.try_add(std::string(meta::keys::constraints::min),
                     meta::common::min<float>(attr));
    metadata.try_add(std::string(meta::keys::constraints::max),
                     meta::common::max<float>(attr));
    metadata.try_add(std::string(meta::keys::ui::label),
                     std::string(i ? "Y" : "X"));
    metadata.try_add(std::string(meta::keys::ui::format),
                     std::string("{:.2f}"));

    // Forward the two hints that change how an axis maps and clamps. Without
    // them a linked pair silently behaves differently from the single value
    // row next to it, which is the failure this control exists to avoid.
    if (const auto *p = attr.metadata().try_value<float>(
            meta::keys::ui::drag_max))
      metadata.try_add(std::string(meta::keys::ui::drag_max), *p);

    if (const auto *p = attr.metadata().try_value<bool>(
            meta::keys::ui::log_scale))
      metadata.try_add(std::string(meta::keys::ui::log_scale), *p);
    RowContext axis_ctx = ctx;
    axis_ctx.theme = &axes_theme_;
    if (ctx.default_value)
      axis_ctx.default_value = [get = ctx.default_value, key = attr.name(), i](
                                   const std::string &) -> std::any
      {
        const auto initial = get(key);
        if (auto pair = std::any_cast<glm::vec2>(&initial)) return (*pair)[i];
        return {};
      };
    sliders_[i] = new ParamSlider(axes_[i], axis_ctx, this);
    layout->addWidget(sliders_[i]);
    connect(sliders_[i],
            &ControlBase::edit_started,
            this,
            [this] { begin_edit(); });
    connect(sliders_[i],
            &ControlBase::value_changed,
            this,
            [this, i]
            {
              value_[i] = sliders_[i]->get();
              if (linked_)
              {
                value_[1 - i] = value_[i];
                sliders_[1 - i]->set(value_[i]);
              }
              notify_value_changed();
            });
    connect(sliders_[i],
            &ControlBase::edit_ended,
            this,
            [this] { end_edit(); });
  }
  connect(link_,
          &QToolButton::toggled,
          this,
          [this](bool linked)
          {
            linked_ = linked;
            state_->try_add(std::string(meta::keys::state::locked_xy), linked)
                ->value() = linked;
            if (linked && value_.y != value_.x)
            {
              begin_edit();
              value_.y = value_.x;
              sliders_[1]->set(value_.y);
              notify_value_changed();
              end_edit();
            }
            on_state_changed();
          });
  set(attr.value());
  on_state_changed();
  setFixedHeight(28 + 2 * theme().metrics.row_height);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}
void LinkedSliders::set(const glm::vec2 &value)
{
  value_ = value;
  if (auto *flag = state_->try_value<bool>(meta::keys::state::locked_xy))
    linked_ = *flag;
  for (int i = 0; i < 2; ++i)
    sliders_[i]->set(value_[i]);
  const QSignalBlocker blocker(link_);
  link_->setChecked(linked_);
  on_state_changed();
}
QSize LinkedSliders::sizeHint() const
{
  return QSize(280, 28 + 2 * theme().metrics.row_height);
}
void LinkedSliders::on_state_changed()
{
  if (!link_ || !sliders_[1]) return;
  link_->setText(linked_ ? "X = Y" : "X / Y");
  link_->setEnabled(!is_locked());
  for (int i = 0; i < 2; ++i)
  {
    sliders_[i]->set_locked(is_locked() || (i == 1 && linked_));
    sliders_[i]->set_modified(is_modified());
  }
}
} // namespace meta::qt::industrial
#endif
