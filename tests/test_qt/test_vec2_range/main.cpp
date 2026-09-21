// Regression test for the glm::vec2 RangeBar widget buttons:
// "Center" must centre the span on zero (clamping to domain bounds if outside).
// Run with QT_QPA_PLATFORM=offscreen.
#include <cstdlib>
#include <iostream>

#include <QApplication>
#include <QPushButton>

#include "meta.hpp"
#include "meta_qt/designs/industrial/industrial.hpp"
#include "meta_qt/meta_widget.hpp"
#include "meta_qt/ui/design_registry.hpp"
#include "meta_qt/widget_renderer.hpp"

static void check(bool ok, const char *msg)
{
  if (!ok)
  {
    std::cerr << "FAIL: " << msg << std::endl;
    std::exit(1);
  }
}

static QPushButton *find_button(QWidget *w, const QString &text)
{
  for (auto *btn : w->findChildren<QPushButton *>())
    if (btn->text() == text) return btn;
  return nullptr;
}

static bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  // Case 1: Domain [0, 1], span 0.2 -> centered on zero is [-0.1, 0.1], clamped
  // -> [0.0, 0.1].
  {
    meta::Attribute<glm::vec2> attr("range", glm::vec2(0.2f, 0.4f));
    attr.metadata().add(meta::keys::ui::widget_type, "RangeBar");
    attr.metadata().add(meta::keys::constraints::min, 0.f);
    attr.metadata().add(meta::keys::constraints::max, 1.f);

    auto *w = meta::qt::WidgetRenderer<glm::vec2>::render(attr, nullptr);
    check(w != nullptr, "WidgetRenderer returned null");

    auto *center_btn = find_button(w, QObject::tr("Center"));
    auto *full_btn = find_button(w, QObject::tr("Full"));
    check(center_btn && full_btn, "Buttons not found");

    center_btn->click();
    check(near(attr.value().x, 0.0f) && near(attr.value().y, 0.1f),
          "Case 1: Center failed");

    full_btn->click();
    check(near(attr.value().x, 0.f) && near(attr.value().y, 1.f),
          "Case 1: Full failed");
    check(!full_btn->isEnabled(), "Case 1: Full should be disabled");

    delete w;
  }

  // Case 2: Symmetric domain [-1, 1], span 0.6 -> centered on zero -> [-0.3,
  // 0.3].
  {
    meta::Attribute<glm::vec2> attr("range", glm::vec2(0.2f, 0.8f));
    attr.metadata().add(meta::keys::ui::widget_type, "RangeBar");
    attr.metadata().add(meta::keys::constraints::min, -1.f);
    attr.metadata().add(meta::keys::constraints::max, 1.f);

    auto *w = meta::qt::WidgetRenderer<glm::vec2>::render(attr, nullptr);
    check(w != nullptr, "WidgetRenderer returned null");

    auto *center_btn = find_button(w, QObject::tr("Center"));
    check(center_btn != nullptr, "Center button not found");

    center_btn->click();
    check(near(attr.value().x, -0.3f) && near(attr.value().y, 0.3f),
          "Case 2: Center failed");

    delete w;
  }

  // Case 3: Asymmetric domain [-0.5, 0.2], span 0.8 -> centered on zero is
  // [-0.4, 0.4], clamped -> [-0.4, 0.2].
  {
    meta::Attribute<glm::vec2> attr("range", glm::vec2(-0.5f, 0.3f));
    attr.metadata().add(meta::keys::ui::widget_type, "RangeBar");
    attr.metadata().add(meta::keys::constraints::min, -0.5f);
    attr.metadata().add(meta::keys::constraints::max, 0.2f);

    auto *w = meta::qt::WidgetRenderer<glm::vec2>::render(attr, nullptr);
    check(w != nullptr, "WidgetRenderer returned null");

    auto *center_btn = find_button(w, QObject::tr("Center"));
    check(center_btn != nullptr, "Center button not found");

    // Initial value [-0.5, 0.3] span = 0.8 -> half_span = 0.4 -> [-0.4, 0.4]
    // clamped to [-0.5, 0.2] -> [-0.4, 0.2]
    center_btn->click();
    check(near(attr.value().x, -0.4f) && near(attr.value().y, 0.2f),
          "Case 3: Center failed");

    delete w;
  }

  // Case 4: Persistence of last_active_value across widget regeneration (stock
  // design).
  {
    meta::Attribute<glm::vec2> attr("range", glm::vec2(0.2f, 0.8f));
    attr.metadata().add(meta::keys::ui::widget_type, "RangeBar");
    attr.metadata().add(meta::keys::constraints::min, 0.f);
    attr.metadata().add(meta::keys::constraints::max, 1.f);
    attr.state().add(meta::keys::state::active, true);

    auto *w = meta::qt::WidgetRenderer<glm::vec2>::render(attr, nullptr);
    check(w != nullptr, "WidgetRenderer returned null");

    auto *toggle_btn = find_button(w, QObject::tr("On"));
    check(toggle_btn != nullptr, "Case 4: Toggle button not found");

    // Toggle off -> value becomes inactive (-1, 0)
    toggle_btn->click();
    check(!attr.state().value<bool>(meta::keys::state::active),
          "Case 4: Should be inactive");
    check(near(attr.value().x, -1.0f) && near(attr.value().y, 0.0f),
          "Case 4: Inactive value expected");

    // Delete the widget to simulate regeneration
    delete w;

    // Regenerate widget with the existing attribute state
    w = meta::qt::WidgetRenderer<glm::vec2>::render(attr, nullptr);
    check(w != nullptr, "Case 4: Re-rendered widget returned null");

    toggle_btn = find_button(w, QObject::tr("Off"));
    check(toggle_btn != nullptr, "Case 4: Off toggle button not found");

    // Toggle on -> should restore last active value (0.2, 0.8)
    toggle_btn->click();
    check(attr.state().value<bool>(meta::keys::state::active),
          "Case 4: Should be active");
    check(near(attr.value().x, 0.2f) && near(attr.value().y, 0.8f),
          "Case 4: Restored active value failed");

    delete w;
  }

  // Case 5: Persistence of last_active_value across widget regeneration
  // (industrial design).
  {
    meta::Attribute<glm::vec2> attr("range", glm::vec2(0.3f, 0.7f));
    attr.metadata().add(meta::keys::ui::widget_type, "RangeBar");
    attr.metadata().add(meta::keys::constraints::min, 0.f);
    attr.metadata().add(meta::keys::constraints::max, 1.f);
    attr.state().add(meta::keys::state::active, true);

    meta::qt::industrial::register_design();

    auto *w = meta::qt::DesignRegistry::instance().render(&attr,
                                                          "industrial",
                                                          {},
                                                          nullptr);
    check(w != nullptr, "Industrial render returned null");

    auto *toggle_btn = find_button(w, QObject::tr("On"));
    check(toggle_btn != nullptr,
          "Case 5: Industrial On toggle button not found");

    toggle_btn->click();
    check(!attr.state().value<bool>(meta::keys::state::active),
          "Case 5: Industrial should be inactive");

    // Delete the widget to simulate regeneration
    delete w;

    // Regenerate industrial widget
    w = meta::qt::DesignRegistry::instance().render(&attr,
                                                    "industrial",
                                                    {},
                                                    nullptr);
    check(w != nullptr, "Case 5: Re-rendered industrial widget returned null");

    toggle_btn = find_button(w, QObject::tr("Off"));
    check(toggle_btn != nullptr,
          "Case 5: Industrial Off toggle button not found");

    toggle_btn->click();
    check(attr.state().value<bool>(meta::keys::state::active),
          "Case 5: Industrial should be active");
    check(near(attr.value().x, 0.3f) && near(attr.value().y, 0.7f),
          "Case 5: Industrial restored active value failed");

    delete w;
  }

  return 0;
}
