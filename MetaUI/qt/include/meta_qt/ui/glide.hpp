/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <QObject>
#include <QVariantAnimation>

namespace meta::qt
{

/** @brief A retargetable eased value, used wherever a control must not snap.
 *
 * Exists as a type rather than as a loose QVariantAnimation because two
 * mistakes around animated values are easy to make and hard to spot:
 *
 * - A running QVariantAnimation ignores a new end value. Retargeting without
 *   stopping first silently does nothing, which reads as "the reset button is
 *   broken". to() always stops first.
 * - Committing a value immediately after starting an animation commits the
 *   *old* value, because the animation has not produced the new one yet. The
 *   commit belongs on finished(), which is why that signal carries the value.
 */
class Glide : public QObject
{
  Q_OBJECT

public:
  explicit Glide(int duration_ms, QObject *parent = nullptr);

  /// Animate towards `target`. Safe to call while already running.
  void to(qreal target);

  /// Move immediately, cancelling any running animation. Emits tick(), not finished().
  void jump(qreal value);

  qreal current() const { return current_; }

  bool running() const;

  void set_duration(int ms);

signals:
  /// Emitted on every animation frame and on jump().
  void tick(qreal value);

  /// Emitted once the animation settles. Carry commits on this, never earlier.
  void finished(qreal value);

private:
  QVariantAnimation animation_;
  qreal             current_ = 0.0;
};

} // namespace meta::qt
