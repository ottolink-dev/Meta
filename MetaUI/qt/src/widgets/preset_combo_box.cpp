/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <algorithm>

#include <QAbstractItemView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "meta/logger.hpp"

#include "meta_qt/designs/industrial/combo.hpp"
#include "meta_qt/widgets/preset_combo_box.hpp"

namespace meta::qt
{

PresetComboBox::PresetComboBox(SnapshotManager *snapshot_manager_,
                               QWidget         *parent)
    : QWidget(parent), snapshot_manager(snapshot_manager_)
{
  Logger::log()->trace("PresetComboBox::PresetComboBox");

  this->combo = new QComboBox(this);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(this->combo);

  this->connect(this->combo,
                QOverload<int>::of(&QComboBox::activated),
                this,
                &PresetComboBox::on_index_activated);

  // --- Context menu for deletion

  this->combo->view()->setContextMenuPolicy(Qt::CustomContextMenu);

  this->connect(this->combo->view(),
                &QWidget::customContextMenuRequested,
                this,
                &PresetComboBox::on_context_menu_requested);

  // update this widget if the underlying data have been modified
  // elsewhere
  connection_ = snapshot_manager->snapshots_modified.subscribe(
      [this]() { this->refresh(); });

  this->populate_combo();
}

void PresetComboBox::set_theme(const Theme &theme)
{
  this->theme_ = theme;
  this->industrial_ = true;

  if (this->combo) this->combo->hide();

  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  this->setFixedHeight(this->theme_.metrics.row_height + 4);
  this->update();
}

void PresetComboBox::set_snapshot_provider(
    std::function<nlohmann::json()> provider)
{
  Logger::log()->trace("PresetComboBox::set_snapshot_provider");

  this->snapshot_provider = std::move(provider);
}

void PresetComboBox::refresh()
{
  Logger::log()->trace("PresetComboBox::refresh");

  if (!this->current_preset.empty() &&
      !this->snapshot_manager->has(this->current_preset))
    this->current_preset.clear();

  this->populate_combo(this->current_preset);

  if (this->industrial_) this->update();
}

void PresetComboBox::set_current_preset(const std::string &name)
{
  Logger::log()->trace("PresetComboBox::set_current_preset: '{}'", name);

  if (!this->snapshot_manager->has(name))
  {
    Logger::log()->error("PresetComboBox: no such preset '{}'", name);
    return;
  }

  this->populate_combo(name);
}

std::string PresetComboBox::get_current_preset() const
{
  return this->current_preset;
}

QSize PresetComboBox::sizeHint() const
{
  if (this->industrial_) return QSize(420, this->theme_.metrics.row_height + 4);

  return QWidget::sizeHint();
}

void PresetComboBox::mouseReleaseEvent(QMouseEvent *event)
{
  if (!this->industrial_)
  {
    QWidget::mouseReleaseEvent(event);
    return;
  }

  if (event->button() == Qt::RightButton)
  {
    if (!this->current_preset.empty())
      this->on_context_menu_requested(event->pos());

    return;
  }

  if (event->button() != Qt::LeftButton) return;

  if (industrial::ComboPopup::should_swallow_reopen()) return;

  this->open_industrial_popup();
}

void PresetComboBox::paintEvent(QPaintEvent *event)
{
  if (!this->industrial_)
  {
    QWidget::paintEvent(event);
    return;
  }

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const Metrics &m = this->theme_.metrics;
  const int      margin = m.section_card_margin;

  // Outer card with section_card_margin (14px)
  const QRect card = this->rect().adjusted(margin, 2, -margin, -2);

  painter.setPen(Qt::NoPen);
  painter.setBrush(this->theme_.section_surface);
  painter.drawRoundedRect(card, m.section_card_radius, m.section_card_radius);

  // Interior combo field
  const int   inset_x = m.section_body_padding_x_narrow;
  const QRect inner_rect = card.adjusted(inset_x, 0, -inset_x, 0);

  const int label_width = int(
      std::clamp<qreal>(inner_rect.width() * m.label_width_ratio,
                        m.label_min_width,
                        m.label_max_width));

  // Label: "Preset"
  painter.setFont(row_label_font());
  painter.setPen(
      this->theme_.state_ink(/* modified */ false, /* locked */ false));
  painter.drawText(QRect(inner_rect.left(),
                         inner_rect.top(),
                         label_width,
                         inner_rect.height()),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   tr("Preset"));

  const QRect field(inner_rect.left() + label_width + m.gap,
                    inner_rect.top() +
                        (inner_rect.height() - m.value_field_height) / 2,
                    inner_rect.width() - label_width - m.gap,
                    m.value_field_height);

  // Field box
  painter.setPen(
      QPen(this->popup_open_ ? this->theme_.accent : this->theme_.field_border,
           1));
  painter.setBrush(this->theme_.field);
  painter.drawRoundedRect(QRectF(field).adjusted(0.5, 0.5, -0.5, -0.5),
                          m.radius,
                          m.radius);

  // Current preset text
  QString display_text = this->current_preset.empty()
                             ? tr("[Save preset...]")
                             : QString::fromStdString(this->current_preset);

  painter.setFont(ui_font(12));
  painter.setPen(this->theme_.ink_primary);
  painter.drawText(field.adjusted(8, 0, -22, 0),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   painter.fontMetrics().elidedText(display_text,
                                                    Qt::ElideRight,
                                                    field.width() - 30));

  // Chevron
  const QPoint centre(field.right() - 12, field.center().y());
  QPainterPath chevron;
  chevron.moveTo(centre.x() - 4, centre.y() - 2);
  chevron.lineTo(centre.x(), centre.y() + 2);
  chevron.lineTo(centre.x() + 4, centre.y() - 2);

  painter.setBrush(Qt::NoBrush);
  painter.setPen(QPen(this->theme_.ink_icon, 1.5));
  painter.drawPath(chevron);
}

void PresetComboBox::open_industrial_popup()
{
  if (!this->snapshot_manager) return;

  std::vector<std::string> names = this->snapshot_manager->names();
  std::sort(names.begin(), names.end());

  auto it = std::find(names.begin(), names.end(), "default");
  if (it != names.end())
  {
    names.erase(it);
    names.insert(names.begin(), "default");
  }

  QStringList items;
  items << tr("[Save preset...]");

  int current_idx = 0;
  for (int i = 0; i < int(names.size()); ++i)
  {
    items << QString::fromStdString(names[i]);
    if (names[i] == this->current_preset) current_idx = i + 1;
  }

  auto *popup = new industrial::ComboPopup(this->theme_,
                                           items,
                                           current_idx,
                                           this);

  this->connect(popup,
                &industrial::ComboPopup::selected,
                this,
                [this, names](int index)
                {
                  if (index == 0)
                  {
                    this->save_new_preset();
                    return;
                  }

                  const int name_idx = index - 1;
                  if (name_idx >= 0 && name_idx < int(names.size()))
                  {
                    const std::string &name = names[name_idx];
                    if (this->snapshot_manager->has(name))
                    {
                      this->current_preset = name;
                      this->update();
                      Q_EMIT this->preset_selected(
                          name,
                          this->snapshot_manager->load(name));
                    }
                  }
                });

  this->connect(popup,
                &QObject::destroyed,
                this,
                [this]()
                {
                  this->popup_open_ = false;
                  this->update();
                });

  this->popup_open_ = true;
  this->update();

  const Metrics &m = this->theme_.metrics;
  const int      margin = m.section_card_margin;
  const QRect    card = this->rect().adjusted(margin, 2, -margin, -2);
  const int      inset_x = m.section_body_padding_x_narrow;
  const QRect    inner_rect = card.adjusted(inset_x, 0, -inset_x, 0);
  const int      label_width = int(
      std::clamp<qreal>(inner_rect.width() * m.label_width_ratio,
                        m.label_min_width,
                        m.label_max_width));

  const QRect field(inner_rect.left() + label_width + m.gap,
                    inner_rect.top() +
                        (inner_rect.height() - m.value_field_height) / 2,
                    inner_rect.width() - label_width - m.gap,
                    m.value_field_height);

  popup->popup_for(QRect(this->mapToGlobal(field.topLeft()), field.size()));
}

void PresetComboBox::on_index_activated(int index)
{
  Logger::log()->trace("PresetComboBox::on_index_activated: index={}", index);

  if (index == PresetComboBox::save_action_index)
  {
    this->save_new_preset();
    return;
  }

  const std::string name = this->combo->itemText(index).toStdString();

  if (!this->snapshot_manager->has(name))
  {
    Logger::log()->error("PresetComboBox::on_index_activated: selected preset "
                         "'{}' no longer exists",
                         name);
    this->populate_combo();
    return;
  }

  Logger::log()->trace(
      "PresetComboBox::on_index_activated: selected preset '{}'",
      name);

  this->current_preset = name;
  Q_EMIT this->preset_selected(name, this->snapshot_manager->load(name));
}

void PresetComboBox::save_new_preset()
{
  Logger::log()->trace("PresetComboBox::save_new_preset");

  bool    ok = false;
  QString text = QInputDialog::getText(this,
                                       tr("Save preset"),
                                       tr("Preset name:"),
                                       QLineEdit::Normal,
                                       QString(),
                                       &ok);

  if (!ok || text.trimmed().isEmpty())
  {
    Logger::log()->trace("PresetComboBox::save_new_preset: cancelled");

    this->populate_combo(this->current_preset);
    return;
  }

  const std::string name = text.trimmed().toStdString();

  if (this->snapshot_manager->has(name))
  {
    const auto ret = QMessageBox::question(
        this,
        tr("Overwrite preset"),
        tr("A preset named \"%1\" already exists. Overwrite it?")
            .arg(QString::fromStdString(name)));

    if (ret != QMessageBox::Yes)
    {
      Logger::log()->trace(
          "PresetComboBox::save_new_preset: overwrite cancelled for '{}'",
          name);

      this->populate_combo(this->current_preset);
      return;
    }
  }

  if (!this->snapshot_provider)
  {
    Logger::log()->error("PresetComboBox::save_new_preset: no snapshot "
                         "provider set, cannot save preset");
    this->populate_combo(this->current_preset);
    return;
  }

  this->snapshot_manager->save(name, this->snapshot_provider());

  Logger::log()->trace("PresetComboBox::save_new_preset: saved '{}'", name);

  this->current_preset = name;
  this->populate_combo(name);

  Q_EMIT this->preset_saved(name);
}

void PresetComboBox::on_context_menu_requested(const QPoint &pos)
{
  Logger::log()->trace("PresetComboBox::on_context_menu_requested");

  if (this->industrial_)
  {
    if (this->current_preset.empty() || this->current_preset == "default")
      return;

    QMenu menu(this);
    menu.setWindowFlag(Qt::NoDropShadowWindowHint);
    menu.setFont(ui_font(12));
    menu.setStyleSheet(
        QString(
            "QMenu { background: %1; color: %2; border: 1px solid %3;"
            " border-radius: 8px; padding: 6px; }"
            "QMenu::item { padding: 7px 18px 7px 10px; border-radius: 4px; }"
            "QMenu::item:selected { background: %4; }"
            "QMenu::separator { height: 1px; background: %3; margin: 5px 8px; "
            "}")
            .arg(this->theme_.bar.name(),
                 this->theme_.ink_primary.name(),
                 this->theme_.field_border.name(),
                 this->theme_.section_header_hover.name()));

    QAction *delete_action = menu.addAction(tr("Delete preset"));
    QAction *selected = menu.exec(this->mapToGlobal(pos));

    if (selected == delete_action)
    {
      Logger::log()->trace(
          "PresetComboBox::on_context_menu_requested: delete '{}'",
          this->current_preset);

      this->delete_preset(this->current_preset);
    }
    return;
  }

  const int index = this->combo->view()->indexAt(pos).row();
  if (index < 0) return;

  const std::string name = this->combo->itemText(index).toStdString();

  // --- ignore special entries
  if (name.empty() || name == "──────────" ||
      index == PresetComboBox::save_action_index)
    return;

  QMenu menu(this);

  QAction *delete_action = menu.addAction(tr("Delete preset"));

  QAction *selected = menu.exec(this->combo->mapToGlobal(pos));

  if (selected == delete_action)
  {
    Logger::log()->trace(
        "PresetComboBox::on_context_menu_requested: delete '{}'",
        name);

    this->delete_preset(name);
  }
}

void PresetComboBox::delete_preset(const std::string &name)
{
  Logger::log()->trace("PresetComboBox::delete_preset: '{}'", name);

  if (name == "default")
  {
    QMessageBox::warning(this,
                         tr("Cannot delete preset"),
                         tr("The default preset cannot be deleted."));
    return;
  }

  if (!this->snapshot_manager->has(name)) return;

  const auto ret = QMessageBox::question(
      this,
      tr("Delete preset"),
      tr("Are you sure you want to delete \"%1\"?")
          .arg(QString::fromStdString(name)));

  if (ret != QMessageBox::Yes)
  {
    Logger::log()->trace("PresetComboBox::delete_preset: cancelled '{}'", name);
    return;
  }

  this->snapshot_manager->erase(name);

  Logger::log()->trace("PresetComboBox::delete_preset: deleted preset '{}'",
                       name);

  if (this->current_preset == name) this->current_preset.clear();

  this->populate_combo(this->current_preset);

  Q_EMIT this->preset_deleted(name);
}

void PresetComboBox::populate_combo(const std::string &select_name)
{
  Logger::log()->trace("PresetComboBox::populate_combo: select='{}'",
                       select_name);

  const QSignalBlocker blocker(this->combo);

  this->combo->clear();

  // --- Save preset entry
  this->combo->addItem(tr("[Save preset...]"));

  std::vector<std::string> names = this->snapshot_manager->names();
  std::sort(names.begin(), names.end());

  // Pull "default" to front
  auto it = std::find(names.begin(), names.end(), "default");
  if (it != names.end())
  {
    names.erase(it);
    names.insert(names.begin(), "default");
  }

  int default_index = -1;

  for (const std::string &name : names)
  {
    if (name == "default") default_index = this->combo->count();

    this->combo->addItem(QString::fromStdString(name));
  }

  // --- separator
  const int sep_index = default_index >= 0 ? default_index + 1
                                           : this->combo->count();

  this->combo->insertItem(sep_index, "──────────");
  this->combo->setItemData(sep_index, 0, Qt::UserRole - 1);

  // --- selection logic
  const std::string &target = select_name.empty() ? this->current_preset
                                                  : select_name;

  int index_to_select = PresetComboBox::save_action_index;

  if (!target.empty())
  {
    const int found = this->combo->findText(QString::fromStdString(target));

    if (found >= 0) index_to_select = found;
  }

  this->combo->setCurrentIndex(index_to_select);

  this->current_preset = (index_to_select == PresetComboBox::save_action_index)
                             ? std::string()
                             : this->combo->itemText(index_to_select)
                                   .toStdString();
}

} // namespace meta::qt
