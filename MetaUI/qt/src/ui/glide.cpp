/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta_qt/ui/glide.hpp"

#include <QEasingCurve>

namespace meta::qt
{

Glide::Glide(int duration_ms, QObject *parent) : QObject(parent)
{
  animation_.setDuration(duration_ms);
  animation_.setEasingCurve(QEasingCurve::OutCubic);

  connect(&animation_,
          &QVariantAnimation::valueChanged,
          this,
          [this](const QVariant &v)
          {
            current_ = v.toReal();
            Q_EMIT tick(current_);
          });

  connect(&animation_,
          &QVariantAnimation::finished,
          this,
          [this]() { Q_EMIT finished(current_); });
}

void Glide::to(qreal target)
{
  if (qFuzzyCompare(target, current_) && !running())
  {
    Q_EMIT finished(current_);
    return;
  }

  // A running animation ignores a retargeted end value -- stop before
  // retargeting or this call silently does nothing.
  animation_.stop();
  animation_.setStartValue(current_);
  animation_.setEndValue(target);
  animation_.start();
}

void Glide::jump(qreal value)
{
  animation_.stop();
  current_ = value;
  Q_EMIT tick(current_);
}

bool Glide::running() const
{
  return animation_.state() == QAbstractAnimation::Running;
}

void Glide::set_duration(int ms) { animation_.setDuration(ms); }

} // namespace meta::qt
