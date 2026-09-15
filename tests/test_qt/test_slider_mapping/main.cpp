// Regression tests for two industrial slider behaviours.
//
// 1. log_scale (Meta issue #55). The mapping used to be discarded whenever the
//    minimum sat at or below the log floor, which includes the very common
//    minimum of zero, so a log attribute drew and dragged as a linear one and
//    nothing said why.
//
// 2. ui.drag_max. The rail may deliberately stop short of what the parameter
//    accepts: dragging is held to the rail, typing goes to the real maximum.
//    This used to key off a maximum of exactly 64, which caught unrelated
//    parameters whose 64 is a hard cap rather than a comfortable rail end.

#include <cmath>
#include <iostream>
#include <limits>

#include <QApplication>
#include <QLineEdit>
#include <QMouseEvent>

#include "meta/core/attribute_container.hpp"
#include "meta/metadata/keys.hpp"

#include "meta_qt/designs/industrial/param_slider.hpp"

using namespace meta::qt;

namespace
{

int failures = 0;

void check(bool ok, const char *message)
{
  if (!ok)
  {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

void flush()
{
  for (int i = 0; i < 8; ++i)
  {
    QApplication::sendPostedEvents();
    QApplication::processEvents();
  }
}

/// The value field a slider owns, so a typed commit can be exercised.
QLineEdit *field_of(QWidget *slider)
{
  return slider->findChild<QLineEdit *>();
}

/// Press, drag and release at `x`, which is how a rail is actually driven.
void click_rail(QWidget *slider, int x)
{
  const QPoint pos(x, slider->height() / 2);

  for (auto type : {QEvent::MouseButtonPress, QEvent::MouseButtonRelease})
  {
    QMouseEvent event(type,
                      QPointF(pos),
                      QPointF(slider->mapToGlobal(pos)),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QApplication::sendEvent(slider, &event);
  }

  flush();
}

meta::Attribute<float> *make_attr(meta::AttributeContainer &container,
                                  const std::string        &name,
                                  float                     value,
                                  float                     min,
                                  float                     max)
{
  auto *attr = container.add(name, value);
  auto &m = attr->metadata();
  m.add(meta::keys::ui::widget_type, std::string("SliderFloat"));
  m.add(meta::keys::ui::label, std::string(name));
  m.add(meta::keys::constraints::min, min);
  m.add(meta::keys::constraints::max, max);
  return attr;
}

} // namespace

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  const Theme theme;
  RowContext  ctx;
  ctx.theme = &theme;

  meta::AttributeContainer container;

  // --- a log rail must not behave like a linear one, even from zero --------
  //
  // Driven by clicking rather than by inspecting the mapping, because the
  // mapping is private and clicking is what a user does. Both rails span
  // 0..100 at the same width, so the same press lands at the same fraction of
  // each, and a log rail must return a far smaller value there.
  {
    auto *linear = make_attr(container, "linear", 1.f, 0.f, 100.f);
    auto *logged = make_attr(container, "logged", 1.f, 0.f, 100.f);
    logged->metadata().add(meta::keys::ui::log_scale, true);

    industrial::ParamSlider linear_slider(*linear, ctx);
    industrial::ParamSlider log_slider(*logged, ctx);
    linear_slider.resize(400, 36);
    log_slider.resize(400, 36);
    flush();

    const int x = 240; // somewhere along the rail, the same on both

    click_rail(&linear_slider, x);
    click_rail(&log_slider, x);

    const float linear_value = linear_slider.get();
    const float log_value = log_slider.get();

    std::cout << "same press: linear=" << linear_value << " log=" << log_value
              << '\n';

    check(linear_value > 0.f, "the linear rail responded to a press");
    check(log_value < linear_value,
          "a log rail returns a smaller value than a linear one at the same "
          "point, i.e. log_scale is actually applied");
  }

  // --- log scale is declined only when the range has no span --------------
  {
    auto *attr = make_attr(container,
                           "log_unbounded",
                           1.f,
                           0.f,
                           std::numeric_limits<float>::max());
    attr->metadata().add(meta::keys::ui::log_scale, true);

    industrial::ParamSlider slider(*attr, ctx);
    slider.resize(400, 36);
    flush();

    slider.set(42.f);
    check(std::abs(slider.get() - 42.f) < 1e-3f,
          "an unbounded log slider still holds the value it was given");
  }

  // --- ui.drag_max: the rail stops short, typing does not ------------------
  {
    auto *attr = make_attr(container, "soft_max", 2.f, 0.f, 512.f);
    attr->metadata().add(meta::keys::ui::drag_max, 64.f);

    industrial::ParamSlider slider(*attr, ctx);
    slider.resize(400, 36);
    flush();

    QLineEdit *field = field_of(&slider);
    check(field != nullptr, "the soft max slider has a value field");

    if (field)
    {
      field->setText("512");
      Q_EMIT field->editingFinished();
      flush();

      check(slider.get() > 64.f,
            "a typed value above the rail maximum is accepted");
      check(std::abs(slider.get() - 512.f) < 1e-2f,
            "a typed value is held to the real maximum, not to the rail");
    }

    // Dragging is still held to the rail, which is the whole point of the key.
    // Pressed near the right hand end of the rail rather than past the widget,
    // because a press outside the rail is correctly ignored and would prove
    // nothing.
    click_rail(&slider, slider.width() - 100);
    check(slider.get() <= 64.f + 1e-3f,
          "dragging stops at the rail maximum even though typing did not");
  }

  // --- no ui.drag_max: the real maximum is the rail maximum ---------------
  //
  // The case the old max == 64 rule broke. Islands is 1..64 and that 64 is a
  // hard cap, so a typed 999 must come back clamped.
  {
    auto *attr = make_attr(container, "islands", 5.f, 1.f, 64.f);

    industrial::ParamSlider slider(*attr, ctx);
    slider.resize(400, 36);
    flush();

    QLineEdit *field = field_of(&slider);
    if (field)
    {
      field->setText("999");
      Q_EMIT field->editingFinished();
      flush();

      check(slider.get() <= 64.f,
            "a hard maximum of 64 still clamps a typed value");
    }
  }

  // --- the readout honours the declared presentation type -----------------
  //
  // HydraulicParticle declares its rates as "{:.2e}" over [1e-6, 1e-1].
  // Rendered fixed, 1e-3 comes out as 0.00100000 and overruns the value field;
  // Otto saw it clipped to a run of zeroes with no decimal point in sight.
  {
    auto *attr = make_attr(container, "drag_rate", 1e-3f, 1e-6f, 1e-1f);
    attr->metadata().add(meta::keys::ui::format, std::string("{:.2e}"));
    attr->metadata().add(meta::keys::ui::log_scale, true);

    industrial::ParamSlider slider(*attr, ctx);
    slider.resize(400, 36);
    flush();

    QLineEdit *field = field_of(&slider);
    check(field != nullptr, "the rate slider has a value field");

    if (field)
    {
      std::cout << "scientific readout: " << field->text().toStdString()
                << '\n';
      check(field->text().contains('e') || field->text().contains('E'),
            "a {:.2e} attribute renders with an exponent");
      check(field->text().size() <= 10,
            "the scientific readout is short enough for the value field");
    }
  }

  // --- a fixed spec still widens rather than rounding a small value away ---
  {
    auto *attr = make_attr(container, "fixed_small", 0.001f, 0.f, 1.f);
    attr->metadata().add(meta::keys::ui::format, std::string("{:.2f}"));

    industrial::ParamSlider slider(*attr, ctx);
    slider.resize(400, 36);
    flush();

    QLineEdit *field = field_of(&slider);
    if (field)
    {
      std::cout << "fixed readout: " << field->text().toStdString() << '\n';
      check(field->text() != "0.00",
            "a small value under a fixed spec does not read as zero");
      check(!field->text().contains('e'),
            "a fixed spec is not promoted to scientific");
    }
  }

  std::cout << "slider mapping checks: failures=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}
