/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <any>
#include <functional>
#include <string>

#include <QWidget>

#include "meta_qt/ui/theme.hpp"

class QWheelEvent;

namespace meta::qt
{

/** @brief What a control is handed at construction.
 *
 * Everything a control needs that is not on the attribute itself. Kept as a
 * struct so adding a field later does not touch every control constructor.
 */
struct RowContext
{
  const Theme *theme = nullptr;

  /** @brief Default value for an attribute key, for the modified/default state.
   *
   * Supplied by the host, because "modified" means `|value - default| > 1e-4`
   * and Meta attributes do not carry their default. An empty std::any (or an
   * unset provider) means "unknown", which is reported as *not* modified --
   * a panel that wrongly shows everything as modified is worse than one that
   * shows nothing as modified.
   */
  std::function<std::any(const std::string &key)> default_value;
};

/** @brief Non-templated base for every attribute control.
 *
 * Carries the parts that cannot live in a template (Q_OBJECT) and the parts
 * every control must get right regardless of design. Two behaviours are
 * enforced here rather than documented, because both have been re-broken:
 *
 * - Wheel events are ignored unless the control has focus, so scrolling the
 *   panel cannot silently change whatever value sits under the cursor.
 *   wheelEvent() is final; override handle_wheel() instead.
 * - is_editing() is set by begin_edit()/end_edit() alongside the matching
 *   signals, so a control cannot raise one without the other. The binder reads
 *   it to stop a model sync from re-seating a value mid-drag.
 */
class ControlBase : public QWidget
{
  Q_OBJECT

public:
  explicit ControlBase(const RowContext &ctx, QWidget *parent = nullptr);

  const Theme      &theme() const { return *theme_; }
  const RowContext &context() const { return ctx_; }

  /// True between begin_edit() and end_edit(); a sync must not fight this.
  bool is_editing() const { return editing_; }

  bool is_locked() const { return locked_; }
  void set_locked(bool locked);

  /// `|value - default| > 1e-4`. Drives text colour only -- never the fill.
  bool is_modified() const { return modified_; }
  void set_modified(bool modified);

signals:
  void edit_started();
  void value_changed();
  void edit_ended();

protected:
  /// Enter the editing state and emit edit_started(). Idempotent.
  void begin_edit();

  /// Leave the editing state and emit edit_ended(). Idempotent.
  void end_edit();

  /// Emit value_changed(). Does not alter the editing state.
  void notify_value_changed();

  /// Called when the state matrix changed; repaint or restyle here.
  virtual void on_state_changed() { update(); }

  /** @brief Elide `text` to `width`, surfacing the full text as a tooltip.
   *
   * Attribute labels routinely overrun the label column. A hard cut is
   * indistinguishable from a genuinely short name, so elide and let the
   * tooltip carry the rest -- but never shadow a tooltip the host already set
   * from ui.tooltip metadata.
   */
  QString elide_label(const QString &text, const QFont &font, int width);

  /// Override this rather than wheelEvent(); only called when focused.
  virtual void handle_wheel(QWheelEvent *event);

  void wheelEvent(QWheelEvent *event) final;

private:
  RowContext   ctx_;
  const Theme *theme_ = nullptr;
  bool         editing_ = false;
  bool         locked_ = false;
  bool         modified_ = false;
};

/** @brief Typed control interface.
 *
 * Typed rather than QVariant-based so glm and Meta types need no metatype
 * registration, and so a mismatched registration fails to compile instead of
 * failing at runtime.
 *
 * Implementations are constructed as `ControlT(Attribute<T> &, const RowContext
 * &, QWidget *)` -- see make_row_factory(). A control reads its own metadata
 * (label, range, format, log scale); the binder deliberately knows none of it.
 */
template <class T> class Control : public ControlBase
{
public:
  using ControlBase::ControlBase;

  virtual T get() const = 0;

  /** @brief Seat a value coming from the model.
   *
   * The binder already suppresses this during an edit, so implementations need
   * not re-check is_editing().
   */
  virtual void set(const T &value) = 0;
};

} // namespace meta::qt
