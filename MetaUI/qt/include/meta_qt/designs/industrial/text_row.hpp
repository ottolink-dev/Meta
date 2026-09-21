/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <string>

#include "meta_common.hpp"

#include "meta_qt/ui/control.hpp"

class QLineEdit;

namespace meta::qt::industrial
{

/** @brief Label and a text field, for a std::string attribute.
 *
 * Covers both SingleLineText and ReadOnlyText, which differ only in whether
 * the field accepts typing. 20 rows in a Hesiod panel.
 *
 * The field stretches from the label column to the right edge rather than
 * using the sliders' narrow value box. A slider's readout is a number a few
 * characters wide sitting beside a rail that needs the room; a text row has no
 * rail and its content is the whole point, so giving it the same 74px box
 * would truncate almost everything worth typing.
 *
 * Chrome comes from field_stylesheet, the same function the slider readouts
 * use, so the two kinds of field are the same object at different widths.
 */
class TextRow : public Control<std::string>
{
  Q_OBJECT

public:
  TextRow(Attribute<std::string> &attr,
          const RowContext       &ctx,
          QWidget                *parent = nullptr);

  /// Any string attribute. There is no metadata this row cannot honour.
  static bool can_render(const Attribute<std::string> &attr);

  std::string get() const override { return value_; }
  void        set(const std::string &value) override;

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void on_state_changed() override;
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  /// Label column, matching the sliders so the two line up down the panel.
  QRect label_rect() const;

  /// Everything to the right of the label.
  QRect field_rect() const;

  void refresh_field();
  void restyle_field(bool editing = false);

  std::string value_;
  std::string label_;

  /** @brief Declared read only by the attribute, as opposed to locked.
   *
   * Separate from is_locked(), which is a runtime state the host drives. A
   * ReadOnlyText is never editable whatever the host says.
   */
  bool read_only_ = false;

  QLineEdit *field_ = nullptr;
};

} // namespace meta::qt::industrial
