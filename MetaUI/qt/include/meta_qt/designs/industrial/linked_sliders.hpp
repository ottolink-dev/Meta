/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#ifdef META_ENABLE_GLM_TYPES
#include "meta_common.hpp"
#include "meta_qt/designs/industrial/param_slider.hpp"
#include "meta_qt/ui/control.hpp"
#include <glm/glm.hpp>
class QToolButton;
namespace meta::qt::industrial
{
// Shared scalar controls give bounded and unbounded pairs identical behavior.
class LinkedSliders : public Control<glm::vec2>
{
  Q_OBJECT
public:
  LinkedSliders(Attribute<glm::vec2> &,
                const RowContext &,
                QWidget * = nullptr);
  static bool can_render(const Attribute<glm::vec2> &) { return true; }
  glm::vec2   get() const override { return value_; }
  void        set(const glm::vec2 &) override;
  QSize       sizeHint() const override;

protected:
  void on_state_changed() override;

private:
  Theme               axes_theme_;
  Attribute<float>    axes_[2] = {{"x", 0.f}, {"y", 0.f}};
  ParamSlider        *sliders_[2] = {nullptr, nullptr};
  QToolButton        *link_ = nullptr;
  AttributeContainer *state_ = nullptr;
  glm::vec2           value_{};
  bool                linked_ = false;
};
} // namespace meta::qt::industrial
#endif
