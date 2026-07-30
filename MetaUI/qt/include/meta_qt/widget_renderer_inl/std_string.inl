/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once

#include <QButtonGroup>
#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "meta/type/type_name.hpp"
#include "meta_common.hpp"
#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widgets/helpers.hpp"

namespace meta::qt
{

namespace helpers
{

// plain_text_height lives in meta_qt/widgets/helpers.hpp (included above).
// It must NOT be redefined here: helpers.cpp defines it non-inline, and a
// second inline definition is an ODR violation that GNU ld resolves silently
// but MSVC rejects outright (LNK2005, COMDAT vs non-COMDAT).

// apply min/max height constraints to a QPlainTextEdit from metadata.
inline void apply_height_constraints(QPlainTextEdit         *te,
                                     Attribute<std::string> &attr,
                                     int                     default_min,
                                     int                     default_max)
{
  const int min_lines = meta::common::try_get<int>(attr,
                                                   "ui.min_lines",
                                                   default_min);
  const int max_lines = meta::common::try_get<int>(attr,
                                                   "ui.max_lines",
                                                   default_max);

  te->setMinimumHeight(plain_text_height(te, min_lines));
  te->setMaximumHeight(plain_text_height(te, max_lines));
}

// create a small right-aligned "Apply" button row.  Returns
// {row_layout, button} - caller adds the row to the parent layout.
inline std::pair<QHBoxLayout *, QPushButton *> make_apply_button(
    QWidget *parent)
{
  auto *apply_btn = new QPushButton(QObject::tr("Apply"), parent);
  apply_btn->setFixedHeight(22);
  apply_btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  auto *btn_row = new QHBoxLayout();
  btn_row->addStretch();
  btn_row->addWidget(apply_btn);

  return {btn_row, apply_btn};
}

} // namespace helpers

template <> struct WidgetRenderer<std::string>
{
  static MetaWidget *render(Attribute<std::string> &attr, QWidget *parent)
  {
    std::string       widget_type = meta::common::widget_type(attr);
    const std::string label_txt = meta::common::label(attr);
    const std::vector<std::string> options = meta::common::allowed_values(attr);
    std::string                   &value = attr.value();

    // default widget type: ComboBox when choices exist,
    // SingleLineText otherwise.
    if (widget_type.empty())
      widget_type = options.empty() ? "SingleLineText" : "ComboBox";

    // multi-line widgets need the label on top → VBoxLayout.
    // Single-line widgets keep the label inline → HBoxLayout (matches
    // existing style).
    const bool needs_vbox = (widget_type == "MultilineText" ||
                             widget_type == "CodeEditor");

    MetaWidget *widget = needs_vbox ? make_meta_widget_vbox(parent)
                                    : make_meta_widget_hbox(parent);

    // retrieve the layout as QBoxLayout so we can call
    // addWidget/addLayout without casting twice.
    auto *layout = static_cast<QBoxLayout *>(widget->layout());

    if (!label_txt.empty())
      layout->addWidget(new QLabel(QString::fromStdString(label_txt), widget));

    if (widget_type == "None") // --- None
    {
      return nullptr;
    }
    else if (widget_type == "SingleLineText") // --- SingleLineText
    {
      const std::string placeholder = meta::common::try_get<std::string>(
          attr,
          "ui.placeholder",
          std::string{});
      const int max_length = meta::common::try_get<int>(attr,
                                                        "ui.max_length",
                                                        0);

      auto *line_edit = new QLineEdit(widget);
      line_edit->setText(QString::fromStdString(value));

      if (!placeholder.empty())
        line_edit->setPlaceholderText(QString::fromStdString(placeholder));

      if (max_length > 0) line_edit->setMaxLength(max_length);

      // line edit and Apply button share one row.
      auto *apply_btn = new QPushButton(QObject::tr("Apply"), widget);
      apply_btn->setFixedHeight(22);
      apply_btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

      auto *row = new QHBoxLayout();
      row->addWidget(line_edit);
      row->addWidget(apply_btn);
      layout->addLayout(row);

      widget->set_sync_from_model(
          [line_edit, &value]()
          {
            const QSignalBlocker blocker(line_edit);
            line_edit->setText(QString::fromStdString(value));
          });

      // stage edits; commit only on Apply / Return.
      auto do_apply = [&attr, line_edit, widget]()
      {
        attr.set_from_any(line_edit->text().toStdString());
        Q_EMIT widget->edit_started();
        Q_EMIT widget->value_changed();
        Q_EMIT widget->edit_ended();
      };

      QObject::connect(apply_btn, &QPushButton::clicked, widget, do_apply);
      QObject::connect(line_edit, &QLineEdit::returnPressed, widget, do_apply);
    }
    else if (widget_type == "MultilineText") // --- MultilineText
    {
      const std::string placeholder = meta::common::try_get<std::string>(
          attr,
          "ui.placeholder",
          std::string{});

      auto *text_edit = new QPlainTextEdit(widget);
      text_edit->setPlainText(QString::fromStdString(value));

      if (!placeholder.empty())
        text_edit->setPlaceholderText(QString::fromStdString(placeholder));

      helpers::apply_height_constraints(text_edit, attr, 4, 12);

      layout->addWidget(text_edit);

      auto [btn_row, apply_btn] = helpers::make_apply_button(widget);
      layout->addLayout(btn_row);

      widget->set_sync_from_model(
          [text_edit, &value]()
          {
            const QSignalBlocker blocker(text_edit);
            text_edit->setPlainText(QString::fromStdString(value));
          });

      QObject::connect(apply_btn,
                       &QPushButton::clicked,
                       widget,
                       [&attr, text_edit, widget]()
                       {
                         attr.set_from_any(
                             text_edit->toPlainText().toStdString());
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       });
    }
    else if (widget_type == "CodeEditor") // --- CodeEditor
    {
      const std::string placeholder = meta::common::try_get<std::string>(
          attr,
          "ui.placeholder",
          std::string{});
      const int tab_width = meta::common::try_get<int>(attr, "ui.tab_width", 4);

      auto *text_edit = new QPlainTextEdit(widget);

      // fixed-pitch font, prefer common coding fonts, fall back to
      // system fixed
      QFont code_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
      for (const char *name : {"JetBrains Mono",
                               "Fira Code",
                               "Cascadia Code",
                               "Consolas",
                               "DejaVu Sans Mono",
                               "Courier New"})
      {
        QFont f(name);
        if (QFontInfo(f).fixedPitch())
        {
          code_font = f;
          break;
        }
      }
      code_font.setPointSize(9);
      text_edit->setFont(code_font);

      // tab stop in pixels = tab_width * advance width of one space.
      const int space_width = QFontMetrics(code_font).horizontalAdvance(
          QLatin1Char(' '));
      text_edit->setTabStopDistance(tab_width * space_width);

      text_edit->setLineWrapMode(QPlainTextEdit::NoWrap);
      text_edit->setPlainText(QString::fromStdString(value));

      if (!placeholder.empty())
        text_edit->setPlaceholderText(QString::fromStdString(placeholder));

      helpers::apply_height_constraints(text_edit, attr, 6, 24);

      layout->addWidget(text_edit);

      auto [btn_row, apply_btn] = helpers::make_apply_button(widget);
      layout->addLayout(btn_row);

      widget->set_sync_from_model(
          [text_edit, &value]()
          {
            const QSignalBlocker blocker(text_edit);
            text_edit->setPlainText(QString::fromStdString(value));
          });

      QObject::connect(apply_btn,
                       &QPushButton::clicked,
                       widget,
                       [&attr, text_edit, widget]()
                       {
                         attr.set_from_any(
                             text_edit->toPlainText().toStdString());
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       });
    }
    else if (widget_type == "ComboBox") // ---
    {
      auto *combo = new QComboBox(widget);
      layout->addWidget(combo);

      int current_index = -1;
      for (size_t i = 0; i < options.size(); ++i)
      {
        combo->addItem(QString::fromStdString(options[i]));
        if (options[i] == value) current_index = static_cast<int>(i);
      }
      if (current_index >= 0) combo->setCurrentIndex(current_index);

      widget->set_sync_from_model(
          [&attr, combo, &value]()
          {
            const QSignalBlocker blocker(combo);

            // Re-pull the allowed-values list: dynamic sources (e.g. the
            // Receive node's tag list) can change without a panel rebuild.
            const std::vector<std::string> current_options =
                meta::common::allowed_values(attr);

            bool items_differ = combo->count() !=
                                static_cast<int>(current_options.size());
            if (!items_differ)
            {
              for (int i = 0; i < combo->count(); ++i)
              {
                if (combo->itemText(i).toStdString() != current_options[i])
                {
                  items_differ = true;
                  break;
                }
              }
            }

            if (items_differ)
            {
              combo->clear();
              for (const auto &opt : current_options)
                combo->addItem(QString::fromStdString(opt));
            }

            const QString v = QString::fromStdString(value);

            for (int i = 0; i < combo->count(); ++i)
            {
              if (combo->itemText(i) == v)
              {
                combo->setCurrentIndex(i);
                return;
              }
            }
          });

      QObject::connect(combo,
                       &QComboBox::currentTextChanged,
                       widget,
                       [&attr, widget](const QString &text)
                       {
                         attr.set_from_any(text.toStdString());
                         Q_EMIT widget->edit_started();
                         Q_EMIT widget->value_changed();
                         Q_EMIT widget->edit_ended();
                       });
    }
    else if (widget_type == "ButtonGrid") // --- ButtonGrid
    {
      int max_cols = 5;
      if (attr.metadata().contains("ui.columns"))
      {
        auto *c = attr.metadata().find("ui.columns");
        max_cols = std::any_cast<int>(c->to_any());
      }

      const int n = static_cast<int>(options.size());
      int ncols = std::min(max_cols, static_cast<int>(std::ceil(std::sqrt(n))));

      auto *grid = new QGridLayout();
      auto *group = new QButtonGroup(widget);

      bool exclusive = true;
      if (attr.metadata().contains("ui.exclusive"))
      {
        auto *m2 = attr.metadata().find("ui.exclusive");
        exclusive = std::any_cast<bool>(m2->to_any());
      }
      group->setExclusive(exclusive);

      for (int i = 0; i < n; ++i)
      {
        const std::string &choice = options[i];
        auto *btn = new QPushButton(QString::fromStdString(choice), widget);
        btn->setCheckable(true);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        if (choice == value) btn->setChecked(true);
        group->addButton(btn);
        grid->addWidget(btn, i / ncols, i % ncols);
      }

      widget->set_sync_from_model(
          [group, &value]()
          {
            const QSignalBlocker blocker(group);

            const QString v = QString::fromStdString(value);

            for (auto *button : group->buttons())
            {
              if (button->text() == v)
              {
                button->setChecked(true);
                return;
              }
            }
          });

      QObject::connect(
          group,
          QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
          widget,
          [&attr, widget](QAbstractButton *button)
          {
            attr.set_from_any(button->text().toStdString());
            Q_EMIT widget->edit_started();
            Q_EMIT widget->value_changed();
            Q_EMIT widget->edit_ended();
          });

      layout->addLayout(grid);
    }
    else // --- ERROR
    {
      layout->addWidget(
          make_error_widget(&attr, "unsupported widget type", widget));
    }

    // connection: attribute changed ==> widget update (dies with the
    // widget destruction)
    widget->connection_ = attr.value_changed.subscribe(
        [widget](std::string) { widget->sync_widget_from_model(); });

    return widget;
  }
};

} // namespace meta::qt
