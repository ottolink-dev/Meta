// Regression test for the int "Input" widget (issue #24): a model value
// outside the current constraints must be clamped back into the model at
// construction, not only in the display. Run with QT_QPA_PLATFORM=offscreen.
#include <cassert>

#include <QApplication>

#include "meta.hpp"
#include "meta_qt/meta_widget.hpp"
#include "meta_qt/widget_renderer.hpp"

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  // Value above the constraint range, as left behind by a file saved before
  // the constraints were tightened.
  meta::Attribute<int> attr("i", 25);
  attr.metadata().add(meta::keys::constraints::min, 0);
  attr.metadata().add(meta::keys::constraints::max, 10);

  auto *w = meta::qt::WidgetRenderer<int>::render(attr, nullptr);
  assert(w);
  assert(attr.value() == 10); // model agrees with the clamped display

  // An in-range value must pass through untouched.
  meta::Attribute<int> attr2("i", 5);
  attr2.metadata().add(meta::keys::constraints::min, 0);
  attr2.metadata().add(meta::keys::constraints::max, 10);

  auto *w2 = meta::qt::WidgetRenderer<int>::render(attr2, nullptr);
  assert(w2);
  assert(attr2.value() == 5);

  delete w;
  delete w2;
  return 0;
}
