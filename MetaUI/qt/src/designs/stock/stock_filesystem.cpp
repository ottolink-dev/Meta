/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "stock_internal.hpp"

#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "meta_common.hpp"
#include "meta_qt/designs/stock/stock.hpp"
#include "meta_qt/meta_widget.hpp"

namespace meta::qt::stock
{

namespace
{

MetaWidget *render_filesystem(AbstractAttribute &abstract_attr,
                              const RowContext &,
                              QWidget *parent)
{
  auto &attr = static_cast<Attribute<std::filesystem::path> &>(abstract_attr);
  std::string       widget_type = meta::common::widget_type(attr);
  const std::string label_txt = meta::common::label(attr);
  const std::string filter = meta::common::file_filter(attr);
  const std::string start_dir_meta = meta::common::try_get<std::string>(
      attr,
      "ui.start_dir",
      "");

  std::filesystem::path &value = attr.value();

  MetaWidget *widget = make_meta_widget_vbox(parent);
  auto       *layout = static_cast<QVBoxLayout *>(widget->layout());

  if (widget_type.empty()) widget_type = "OpenFile";

  const bool is_open_file = (widget_type == "OpenFile");
  const bool is_save_file = (widget_type == "SaveFile");
  const bool is_directory = (widget_type == "Directory");

  if (widget_type == "None")
  {
    return nullptr;
  }
  else if (is_open_file || is_save_file || is_directory)
  {
    if (!label_txt.empty())
    {
      layout->addWidget(new QLabel(QString::fromStdString(label_txt), widget));
    }

    auto *row = new QHBoxLayout();

    auto *line_edit = new QLineEdit(widget);
    line_edit->setReadOnly(true);
    line_edit->setPlaceholderText(is_directory
                                      ? QObject::tr("No folder selected")
                                      : QObject::tr("No file selected"));
    line_edit->setText(QString::fromStdString(value.string()));

    widget->set_sync_from_model(
        [line_edit, &value]()
        {
          const QSignalBlocker blocker(line_edit);
          line_edit->setText(QString::fromStdString(value.string()));
        });

    auto *browse_button = new QPushButton(QObject::tr("…"), widget);
    browse_button->setFixedWidth(28);

    auto *clear_button = new QPushButton(QObject::tr("✕"), widget);
    clear_button->setFixedWidth(24);
    clear_button->setToolTip(QObject::tr("Clear"));

    row->addWidget(line_edit);
    row->addWidget(browse_button);
    row->addWidget(clear_button);

    layout->addLayout(row);

    QObject::connect(
        browse_button,
        &QPushButton::clicked,
        widget,
        [&value,
         &attr,
         widget,
         line_edit,
         is_save_file,
         is_directory,
         filter_str = QString::fromStdString(filter),
         meta_dir = QString::fromStdString(start_dir_meta)]()
        {
          const QString start_dir =
              !meta_dir.isEmpty()
                  ? meta_dir
                  : (!value.empty()
                         ? QString::fromStdString(value.parent_path().string())
                         : QDir::homePath());

          QString picked;

          if (is_directory)
          {
            picked = QFileDialog::getExistingDirectory(
                widget,
                QObject::tr("Select folder"),
                start_dir,
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
          }
          else if (is_save_file)
          {
            picked = QFileDialog::getSaveFileName(widget,
                                                  QObject::tr("Save file"),
                                                  start_dir,
                                                  filter_str);
          }
          else
          {
            picked = QFileDialog::getOpenFileName(widget,
                                                  QObject::tr("Open file"),
                                                  start_dir,
                                                  filter_str);
          }

          if (picked.isEmpty()) return;

          attr.set_from_any(std::filesystem::path(picked.toStdString()));
          line_edit->setText(picked);

          Q_EMIT widget->edit_started();
          Q_EMIT widget->value_changed();
          Q_EMIT widget->edit_ended();
        });

    QObject::connect(clear_button,
                     &QPushButton::clicked,
                     widget,
                     [&attr, widget, line_edit]()
                     {
                       attr.set_from_any(std::filesystem::path{});
                       line_edit->clear();

                       Q_EMIT widget->edit_started();
                       Q_EMIT widget->value_changed();
                       Q_EMIT widget->edit_ended();
                     });
  }
  else
  {
    layout->addWidget(
        make_error_widget(&attr, "unsupported widget type", widget));
  }

  widget->connection_ = attr.value_changed.subscribe(
      [widget](std::filesystem::path) { widget->sync_widget_from_model(); });

  return widget;
}

} // namespace

void register_stock_filesystem(DesignRegistry &registry)
{
  const std::type_index type = std::type_index(typeid(std::filesystem::path));
  registry.add(kDesignName, type, "OpenFile", render_filesystem);
  registry.add(kDesignName, type, "SaveFile", render_filesystem);
  registry.add(kDesignName, type, "Directory", render_filesystem);
  registry.add(kDesignName, type, kAnyWidgetType, render_filesystem);
}

} // namespace meta::qt::stock
