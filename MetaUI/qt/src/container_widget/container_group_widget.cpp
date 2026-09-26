/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <algorithm>
#include <cmath>
#include <functional>

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QVBoxLayout>

#include "meta/logger.hpp"

#include "meta_qt/container_group_widget.hpp"
#include "meta_qt/container_widget.hpp"
#include "meta_qt/ui/theme.hpp"

namespace meta::qt
{

namespace
{

/** @brief The group switch drawn in a design's own colours.
 *
 * A rounded track with one equal segment per container and a highlight that
 * slides to the chosen one. Replaces QTabWidget's tabs for designs with their
 * own chrome: native tabs sit on the panel like a foreign widget (square, with
 * a pane frame around the cards), while this reads as part of the panel.
 */
class SegmentedTabs final : public QWidget
{
public:
  SegmentedTabs(const Theme &theme, QWidget *parent) : QWidget(parent), theme(theme)
  {
    this->setAttribute(Qt::WA_Hover);
    this->setMouseTracking(true);
    this->setCursor(Qt::PointingHandCursor);
    this->setFixedHeight(34);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    this->slide = new QVariantAnimation(this);
    this->slide->setDuration(200);
    this->slide->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(this->slide,
                     &QVariantAnimation::valueChanged,
                     this,
                     [this](const QVariant &v)
                     {
                       this->position = v.toReal();
                       this->update();
                     });
  }

  void add(const QString &label) { this->labels.push_back(label); }
  int  count() const { return int(this->labels.size()); }
  QString text(int i) const { return this->labels.at(i); }

  void set_current(int index, bool animate)
  {
    if (index < 0 || index >= this->count())
      return;
    this->current = index;
    this->slide->stop();
    if (!animate || !this->isVisible())
    {
      this->position = index;
      this->update();
      return;
    }
    this->slide->setStartValue(this->position);
    this->slide->setEndValue(qreal(index));
    this->slide->start();
  }

  std::function<void(int)> on_selected;

protected:
  void paintEvent(QPaintEvent *) override
  {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF track = QRectF(this->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal  r = 9.0;

    p.setPen(QPen(this->theme.rail_well_border, 1));
    p.setBrush(this->theme.rail_well);
    p.drawRoundedRect(track, r, r);

    if (this->labels.empty())
      return;

    const qreal inset = 3.0;
    const qreal w = (track.width() - 2 * inset) / this->count();

    // hover, then the sliding highlight
    if (this->hovered >= 0 && this->hovered != this->current)
    {
      QColor hover = this->theme.section_surface;
      hover.setAlphaF(0.35);
      p.setPen(Qt::NoPen);
      p.setBrush(hover);
      p.drawRoundedRect(this->segment(this->hovered, w, inset), r - inset, r - inset);
    }

    {
      const QRectF a = this->segment(0, w, inset);
      const QRectF knob(a.left() + this->position * w, a.top(), a.width(), a.height());
      p.setPen(QPen(this->theme.field_border, 1));
      p.setBrush(this->theme.section_surface);
      p.drawRoundedRect(knob.adjusted(0.5, 0.5, -0.5, -0.5), r - inset, r - inset);
    }

    p.setFont(ui_font(12, true));
    for (int i = 0; i < this->count(); ++i)
    {
      const QRectF cell = this->segment(i, w, inset);
      // ink follows the highlight as it passes, rather than switching at once
      const qreal  on = std::clamp(1.0 - std::abs(this->position - i), 0.0, 1.0);
      const QColor dim = i == this->hovered ? this->theme.ink_primary : this->theme.ink_dim;
      p.setPen(QColor::fromRgbF(dim.redF() + (this->theme.ink_primary.redF() - dim.redF()) * on,
                                dim.greenF() +
                                    (this->theme.ink_primary.greenF() - dim.greenF()) * on,
                                dim.blueF() + (this->theme.ink_primary.blueF() - dim.blueF()) * on));
      const QString text = p.fontMetrics().elidedText(this->labels[i],
                                                      Qt::ElideRight,
                                                      int(cell.width() - 10));
      p.drawText(cell, Qt::AlignCenter, text);
    }
  }

  void mouseMoveEvent(QMouseEvent *event) override
  {
    const int index = this->index_at(event->position());
    if (index != this->hovered)
    {
      this->hovered = index;
      this->update();
    }
  }

  void leaveEvent(QEvent *event) override
  {
    this->hovered = -1;
    this->update();
    QWidget::leaveEvent(event);
  }

  void mousePressEvent(QMouseEvent *event) override
  {
    const int index = this->index_at(event->position());
    if (event->button() != Qt::LeftButton || index < 0 || index == this->current)
      return;
    this->set_current(index, true);
    if (this->on_selected)
      this->on_selected(index);
  }

private:
  QRectF segment(int i, qreal w, qreal inset) const
  {
    return QRectF(inset + i * w, inset, w, this->height() - 2 * inset);
  }

  int index_at(const QPointF &pos) const
  {
    if (this->labels.empty())
      return -1;
    const qreal w = (this->width() - 6.0) / this->count();
    const int   i = int(std::floor((pos.x() - 3.0) / w));
    return (i >= 0 && i < this->count()) ? i : -1;
  }

  const Theme          &theme;
  std::vector<QString>  labels;
  int                   current = 0;
  int                   hovered = -1;
  qreal                 position = 0.0;
  QVariantAnimation    *slide = nullptr;
};

} // namespace

ContainerGroupWidget::ContainerGroupWidget(meta::ContainerGroup  &group,
                                           ContainerRenderOptions options,
                                           QWidget               *parent)
    : MetaWidget(parent), group(group), options(options)
{
  Logger::log()->trace("ContainerGroupWidget::ContainerGroupWidget");

  auto *root = new QVBoxLayout(this);
  this->setLayout(root);

  // designs with their own chrome get a segmented switch in their colours
  // (stock keeps Qt's tabs: it is the unmodified-Qt reference)
  if (options.group_switch_mode == GroupSwitchMode::GSM_TABS &&
      options.design != "stock" && options.row_context.theme)
  {
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(10);

    auto *bar = new SegmentedTabs(*options.row_context.theme, this);
    this->segments = bar;
    auto *bar_row = new QHBoxLayout();
    bar_row->setContentsMargins(14, 4, 14, 0); // aligned with the section cards
    bar_row->addWidget(bar);
    root->addLayout(bar_row);

    stacked = new QStackedWidget(this);
    root->addWidget(stacked);

    for (const auto &key : group.insertion_order())
    {
      bar->add(QString::fromStdString(key));
      auto *w = build_container_widget(key);
      pages[key] = w;
      stacked->addWidget(w);
    }

    bar->on_selected = [this, bar](int index)
    {
      const std::string new_current = bar->text(index).toStdString();
      Logger::log()->trace("ContainerGroupWidget: switching to '{}'", new_current);
      this->group.set_current(new_current);
      this->stacked->setCurrentIndex(index);
      Q_EMIT current_container_changed(new_current);
    };
  }
  else if (options.group_switch_mode == GroupSwitchMode::GSM_TABS)
  {
    tabs = new QTabWidget(this);
    root->addWidget(tabs);

    for (const auto &key : group.insertion_order())
    {
      Logger::log()->trace(
          "ContainerGroupWidget::ContainerGroupWidget: adding tab '{}'",
          key);

      auto *w = build_container_widget(key);
      pages[key] = w;
      tabs->addTab(w, QString::fromStdString(key));
    }

    connect(
        tabs,
        &QTabWidget::currentChanged,
        this,
        [this](int index)
        {
          if (index < 0 || index >= tabs->count()) return;
          const std::string new_current = tabs->tabText(index).toStdString();

          Logger::log()->trace("ContainerGroupWidget: switching to '{}'",
                               new_current);

          this->group.set_current(new_current);
          Q_EMIT current_container_changed(new_current);
        });
  }
  else
  {
    combo = new QComboBox(this);
    root->addWidget(combo);

    stacked = new QStackedWidget(this);
    root->addWidget(stacked);

    for (const auto &key : group.insertion_order())
    {
      Logger::log()->trace(
          "ContainerGroupWidget::ContainerGroupWidget: adding page '{}'",
          key);

      combo->addItem(QString::fromStdString(key));

      auto *w = build_container_widget(key);
      pages[key] = w;
      stacked->addWidget(w);
    }

    connect(combo,
            &QComboBox::currentTextChanged,
            this,
            [this](const QString &text)
            {
              const std::string new_current = text.toStdString();

              Logger::log()->trace("ContainerGroupWidget: switching to '{}'",
                                   new_current);

              this->group.set_current(new_current);
              sync_stack();

              Q_EMIT current_container_changed(new_current);
            });
  }

  // set initial
  sync_stack();

  // pass through for the synching from the model (the
  // ContainerGroupWidget is derived from a MetaWidget)
  set_sync_from_model([this]() { this->on_sync_meta_widgets_from_model(); });
}

QWidget *ContainerGroupWidget::build_container_widget(const std::string &key)
{
  Logger::log()->trace("ContainerGroupWidget::build_container_widget: '{}'",
                       key);

  auto *container = group.find(key);

  if (!container)
  {
    Logger::log()->warn("ContainerGroupWidget::build_container_widget: "
                        "container '{}' not found",
                        key);

    return new QWidget(); // dummy
  }

  MetaWidget *container_widget = meta::qt::render(*container, options);

  // pass-through signals
  connect(container_widget,
          &MetaWidget::edit_started,
          this,
          &MetaWidget::edit_started);

  connect(container_widget,
          &MetaWidget::value_changed,
          this,
          &MetaWidget::value_changed);

  connect(container_widget,
          &MetaWidget::edit_ended,
          this,
          &MetaWidget::edit_ended);

  return container_widget;
}

void ContainerGroupWidget::on_sync_meta_widgets_from_model()
{
  Logger::log()->trace("ContainerGroupWidget::on_sync_meta_widgets_from_model");

  for (const auto &[key, widget] : pages)
  {
    if (auto *meta_widget = qobject_cast<MetaWidget *>(widget))
      meta_widget->sync_widget_from_model();
  }
}

void ContainerGroupWidget::sync_stack()
{
  Logger::log()->trace("ContainerGroupWidget::sync_stack");

  std::optional<std::string> current_name = group.current_container_name();

  if (!current_name)
  {
    Logger::log()->trace(
        "ContainerGroupWidget::sync_stack: no current container");

    return;
  }

  if (auto *bar = static_cast<SegmentedTabs *>(this->segments))
  {
    for (int i = 0; i < bar->count(); ++i)
      if (bar->text(i).toStdString() == *current_name)
      {
        bar->set_current(i, true);
        stacked->setCurrentIndex(i);
        return;
      }

    Logger::log()->warn(
        "ContainerGroupWidget::sync_stack: container '{}' not found in the switch",
        *current_name);
  }
  else if (tabs)
  {
    for (int i = 0; i < tabs->count(); ++i)
    {
      if (tabs->tabText(i).toStdString() == *current_name)
      {
        Logger::log()->trace(
            "ContainerGroupWidget::sync_stack: current='{}' index={}",
            *current_name,
            i);

        tabs->setCurrentIndex(i);
        return;
      }
    }

    Logger::log()->warn(
        "ContainerGroupWidget::sync_stack: container '{}' not found in tabs",
        *current_name);
  }
  else if (combo && stacked)
  {
    int index = combo->findText(QString::fromStdString(*current_name));

    if (index >= 0)
    {
      Logger::log()->trace(
          "ContainerGroupWidget::sync_stack: current='{}' index={}",
          *current_name,
          index);

      stacked->setCurrentIndex(index);
      combo->setCurrentIndex(index);
    }
    else
    {
      Logger::log()->warn(
          "ContainerGroupWidget::sync_stack: container '{}' not found in combo",
          *current_name);
    }
  }
}

// --- Function

MetaWidget *render(meta::ContainerGroup  &group,
                   ContainerRenderOptions options,
                   QWidget               *parent,
                   bool                   render_single_group_as_a_container)
{
  Logger::log()->trace("ContainerGroupWidget::render");

  if (group.size() == 0)
  {
    Logger::log()->error("render / meta::ContainerGroup: empty group");
    return nullptr;
  }

  // group with 1 container => flatten to a single attribute container
  // if requested
  if (render_single_group_as_a_container && group.size() == 1)
  {
    auto &containers = group.containers();
    return render(*containers.begin()->second, options, parent);
  }

  // default
  return new ContainerGroupWidget(group, options, parent);
}

} // namespace meta::qt
