/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <string>
#include <vector>

#include <QRect>
#include <QStringList>
#include <QWidget>

#include "meta_common.hpp"

#include "meta_qt/ui/control.hpp"

class QVariantAnimation;

namespace meta::qt::industrial
{

/** @brief Option list shown when a combo is open.
 *
 * A Qt::Popup of our own rather than QComboBox's view, because the stock popup
 * composites a frame and an item view that are styled separately: mid-open you
 * see one surface and once settled another, and the two rarely agree on colour.
 * Owning the whole surface is the only way to make it one thing.
 *
 * Four separate bugs live in Qt popups, all handled here:
 *
 * - Opening on press means the matching release, which lands outside, dismisses
 *   it again. Callers must open on *release*.
 * - Overriding mousePressEvent suppresses Qt's built-in "press outside
 *   dismisses", so an outside press has to be handled explicitly.
 * - destroyed() is too late to guard against reopening; hideEvent is not.
 * - Clicking the field while open goes to the popup, which closes, and the same
 *   click then reaches the field and reopens it, so it never appears to close.
 *   should_swallow_reopen() exists for exactly that.
 */
class ComboPopup : public QWidget
{
  Q_OBJECT

public:
  ComboPopup(const Theme       &theme,
             const QStringList &items,
             int                current,
             QWidget           *parent);

  /// Show below `field_global`, flipping above when there is no room below.
  void popup_for(const QRect &field_global);

  /// True while a click should be ignored because it is the reopen half of a
  /// close.
  static bool should_swallow_reopen();

signals:
  void selected(int index);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void hideEvent(QHideEvent *event) override;

private:
  int index_at(const QPoint &pos) const;
  int row_height() const;

  /// Portion of the fixed-size window currently revealed by the open animation.
  QRect card_rect() const;

  const Theme       *theme_ = nullptr;
  QStringList        items_;
  int                current_ = -1;
  int                hovered_ = -1;
  QVariantAnimation *open_animation_ = nullptr;
  int                full_height_ = 0;
  int                revealed_ = 0;
  bool               flipped_ = false;
};

/// Shared closed-state painting for both combo flavours.
void paint_combo_field(QWidget       &widget,
                       const Theme   &theme,
                       const QString &label,
                       const QString &value,
                       bool           open,
                       bool           modified,
                       bool           locked);

/** @brief Dropdown for an int attribute carrying enum_items.
 *
 * EnumComboBox, roughly 5% of the rows in a Hesiod node panel.
 */
class EnumCombo : public Control<int>
{
  Q_OBJECT

public:
  EnumCombo(Attribute<int>   &attr,
            const RowContext &ctx,
            QWidget          *parent = nullptr);

  /// Needs enum_items to have anything to show.
  static bool can_render(const Attribute<int> &attr);

  int  get() const override { return value_; }
  void set(const int &value) override;

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void on_state_changed() override { update(); }

private:
  void open_popup();

  int                                      value_ = 0;
  std::vector<std::pair<int, std::string>> items_;
  std::string                              label_;
  bool                                     open_ = false;
};

/** @brief Dropdown for a string attribute carrying allowed_values.
 *
 * ComboBox, plus the ButtonGrid preset, which resolves to the same control.
 */
class StringCombo : public Control<std::string>
{
  Q_OBJECT

public:
  StringCombo(Attribute<std::string> &attr,
              const RowContext       &ctx,
              QWidget                *parent = nullptr);

  static bool can_render(const Attribute<std::string> &attr);

  std::string get() const override { return value_; }
  void        set(const std::string &value) override;

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void on_state_changed() override { update(); }

private:
  void open_popup();

  std::string              value_;
  std::vector<std::string> items_;
  std::string              label_;
  bool                     open_ = false;
};

} // namespace meta::qt::industrial
