// Regression test for the glm::vec2 RangeBar widget buttons (issue #25):
// "Center" must centre the span on the domain midpoint, and buttons that
// cannot change the value must be disabled. Run with
// QT_QPA_PLATFORM=offscreen.
#include <cassert>
#include <cmath>

#include <QApplication>
#include <QPushButton>

#include "meta.hpp"
#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widget_renderer.hpp"

static QPushButton *find_button(QWidget *w, const QString &text)
{
  for (auto *btn : w->findChildren<QPushButton *>())
    if (btn->text() == text) return btn;
  return nullptr;
}

static bool near(float a, float b)
{
  return std::fabs(a - b) < 1e-5f;
}

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  meta::Attribute<glm::vec2> attr("range", glm::vec2(0.2f, 0.4f));
  attr.metadata().add(meta::keys::ui::widget_type, "RangeBar");
  attr.metadata().add(meta::keys::constraints::min, 0.f);
  attr.metadata().add(meta::keys::constraints::max, 1.f);

  auto *w = meta::qt::WidgetRenderer<glm::vec2>::render(attr, nullptr);
  assert(w);

  auto *center_btn = find_button(w, QObject::tr("Center"));
  auto *full_btn = find_button(w, QObject::tr("Full"));
  assert(center_btn && full_btn);

  // Span 0.2 centred on the domain midpoint 0.5 -> [0.4, 0.6].
  // (Pre-fix this centred on 0 and slammed the range to [0.0, 0.2].)
  center_btn->click();
  assert(near(attr.value().x, 0.4f) && near(attr.value().y, 0.6f));

  // A full-domain range: "Full" is a no-op and "Center" cannot shift the
  // span — both must be disabled.
  full_btn->click();
  assert(near(attr.value().x, 0.f) && near(attr.value().y, 1.f));
  assert(!full_btn->isEnabled());
  assert(!center_btn->isEnabled());

  delete w;
  return 0;
}
