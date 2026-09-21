#include "meta/ext/array/array.hpp"
#include "meta/ext/color_gradient/color_gradient.hpp"
#include "meta/ext/color_gradient/gradient_library.hpp"
#include "meta_qt/designs/industrial/industrial.hpp"
#include "meta_qt/designs/industrial/int_slider.hpp"
#include "meta_qt/designs/industrial/linked_sliders.hpp"
#include "meta_qt/designs/industrial/param_slider.hpp"
#include "meta_qt/designs/industrial/section.hpp"
#include "meta_qt/ui/design_registry.hpp"
#include "meta_qt/ui/number_format.hpp"
#include "meta_qt/widgets/array_canvas.hpp"
#include "meta_qt/widgets/points_canvas.hpp"
#include "meta_qt/widgets/range_bar.hpp"
#include <QApplication>
#include <QDir>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>
#include <cmath>
#include <iostream>
using namespace meta::qt;
namespace
{
int  failures = 0;
void check(bool ok, const char *message)
{
  if (!ok)
  {
    ++failures;
    std::cerr << message << '\n';
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
template <class T>
void describe(meta::Attribute<T> &a, const char *kind, const char *label)
{
  a.metadata().add(meta::keys::ui::widget_type, std::string(kind));
  a.metadata().add(meta::keys::ui::label, std::string(label));
}
void mouse(QWidget *w, QEvent::Type type, QPoint pos)
{
  QMouseEvent event(type,
                    QPointF(pos),
                    QPointF(w->mapToGlobal(pos)),
                    Qt::LeftButton,
                    Qt::LeftButton,
                    Qt::NoModifier);
  QApplication::sendEvent(w, &event);
}
} // namespace
int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  Theme        theme;
  theme.accent = QColor("#5f85ab");
  QPalette palette = app.palette();
  palette.setColor(QPalette::Window, theme.section_surface);
  palette.setColor(QPalette::WindowText, theme.ink_primary);
  app.setPalette(palette);
  RowContext ctx;
  ctx.theme = &theme;
  industrial::register_design();
  auto &registry = DesignRegistry::instance();
  {
    meta::Attribute<float> scalar("scalar", 512.f);
    describe(scalar, "Slider", "Scalar");
    // The rail ends at 64, the parameter accepts far more. Declared through
    // ui.drag_max now: this used to be inferred from a maximum of exactly 64,
    // which caught unrelated parameters whose 64 is a hard cap.
    scalar.metadata().add(meta::keys::constraints::min, 0.f);
    scalar.metadata().add(meta::keys::constraints::max, 4096.f);
    scalar.metadata().add(meta::keys::ui::drag_max, 64.f);
    industrial::ParamSlider slider(scalar, ctx);
    check(slider.get() == 512.f, "initial float above drag range was lost");
    slider.set(1024.f);
    check(slider.get() == 1024.f, "float above drag range was lost on refresh");
    auto *field = slider.findChild<QLineEdit *>();
    field->setText("512");
    QMetaObject::invokeMethod(field, "editingFinished");
    check(slider.get() == 512.f, "typed float capped at drag limit");
    slider.resize(400, slider.height());
    slider.show();
    flush();
    mouse(&slider, QEvent::MouseButtonPress, QPoint(295, slider.height() / 2));
    mouse(&slider, QEvent::MouseMove, QPoint(800, slider.height() / 2));
    mouse(&slider,
          QEvent::MouseButtonRelease,
          QPoint(800, slider.height() / 2));
    check(slider.get() <= 64.f, "float drag exceeded 64");
    meta::Attribute<int> integer("integer", 512);
    describe(integer, "Slider", "Integer");
    integer.metadata().add(meta::keys::constraints::min, 0);
    integer.metadata().add(meta::keys::constraints::max, 4096);
    integer.metadata().add(meta::keys::ui::drag_max, 64);
    industrial::IntSlider ints(integer, ctx);
    check(ints.get() == 512, "initial integer above drag range was lost");
    ints.set(1024);
    check(ints.get() == 1024, "integer refresh capped at drag limit");
    auto *int_field = ints.findChild<QLineEdit *>();
    int_field->setText("512");
    QMetaObject::invokeMethod(int_field, "editingFinished");
    check(ints.get() == 512, "typed integer capped at drag limit");
  }
  check(display_float(.001f) == "0.001", "small float displayed as zero");
  check(display_float(-.0001f) == "-0.0001",
        "negative small float lost precision");
  check(display_float(2.f) == "2.00",
        "ordinary float must retain two decimals");
  check(display_float(.00125f) == "0.00125", "small fractional detail lost");
  for (bool bounded : {false, true})
  {
    meta::Attribute<glm::vec2> frequency("frequency", glm::vec2(4, 5));
    describe(frequency, "LinkedSliders", "Spatial Frequency");
    if (bounded)
    {
      frequency.metadata().add(meta::keys::constraints::min, 0.f);
      frequency.metadata().add(meta::keys::constraints::max, 100.f);
    }
    std::unique_ptr<MetaWidget> row(
        registry.render(&frequency, "industrial", ctx));
    check(row->findChild<industrial::LinkedSliders *>() != nullptr,
          "frequency fell back to stock");
    auto fields = row->findChildren<QLineEdit *>();
    check(fields.size() == 2, "linked pair must expose two fields");
    fields[0]->setText("0.001");
    QMetaObject::invokeMethod(fields[0], "editingFinished");
    check(frequency.value().x == .001f, "typed float not committed exactly");
    auto *link = row->findChild<QToolButton *>();
    link->click();
    check(frequency.value().y == frequency.value().x, "link must copy X to Y");
    fields[0]->setText("0.0001");
    QMetaObject::invokeMethod(fields[0], "editingFinished");
    check(frequency.value().x == .0001f && frequency.value().y == .0001f,
          "linked precision lost");
    link->click();
    fields[1]->setText("0.002");
    QMetaObject::invokeMethod(fields[1], "editingFinished");
    check(frequency.value().x == .0001f && frequency.value().y == .002f,
          "unlinked axes not independent");
  }
  meta::Attribute<std::vector<glm::vec3>> path(
      "path",
      {{.15f, .3f, .7f}, {.7f, .7f, 1.f}, {.75f, .2f, .5f}});
  describe(path, "PathEditor", "Path");
  meta::Attribute<meta::Array> brush(
      "brush",
      meta::Array{{256, 256}, std::vector<float>(256 * 256, 0)});
  describe(brush, "ArrayEditor", "Brush");
  brush.metadata().add(meta::keys::ui::width, 256);
  brush.metadata().add(meta::keys::ui::height, 256);
  meta::Attribute<glm::vec2> frequency("frequency", glm::vec2(4));
  describe(frequency, "LinkedSliders", "Spatial Frequency");
  frequency.state().add(meta::keys::state::locked_xy, true);
  meta::Attribute<glm::vec2> range("range", glm::vec2(.2f, .8f));
  describe(range, "RangeBar", "Remap Range");
  range.metadata().add(meta::keys::constraints::min, 0.f);
  range.metadata().add(meta::keys::constraints::max, 1.f);
  range.state().add(meta::keys::state::active, true);
  meta::Attribute<meta::ColorGradient> gradient("gradient",
                                                meta::ColorGradient{});
  describe(gradient, "GradientEditor", "Gradient");
  meta::GradientPresets presets;
  for (int i = 0; i < 20; ++i)
    presets.presets.push_back(
        {"Preset " + std::to_string(i),
         {{0, {0.f, float(i) / 20, .2f, 1}}, {1, {1, 1, 1, 1}}}});
  gradient.metadata().add(meta::keys::ui::presets, presets);
  meta::GradientLibrary::instance().set_path(
      std::filesystem::path(QDir::tempPath().toStdString()) /
      "hesiod-editor-check-gradients.json");
  QScrollArea scroll;
  scroll.setAttribute(Qt::WA_DontShowOnScreen);
  scroll.setWidgetResizable(true);
  auto *page = new QWidget;
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(2);
  layout->setAlignment(Qt::AlignTop);
  std::vector<MetaWidget *> rows;
  for (auto *attr : std::vector<meta::AbstractAttribute *>{&frequency,
                                                           &path,
                                                           &brush,
                                                           &range,
                                                           &gradient})
  {
    auto *section = new industrial::Section("Parameters", theme);
    auto *row = registry.render(attr, "industrial", ctx);
    rows.push_back(row);
    section->content_layout->addWidget(row);
    section->set_expanded(true);
    layout->addWidget(section);
  }
  scroll.setWidget(page);
  scroll.resize(440, 720);
  scroll.show();
  flush();
  auto *points = page->findChild<PointsCanvas *>();
  auto *paint = page->findChild<ArrayCanvas *>();
  {
    const auto original = path.value();
    const int  side = points->width() - 20;
    mouse(
        points,
        QEvent::MouseMove,
        QPoint(10 + int(.15f * side), points->height() - 11 - int(.3f * side)));
    QKeyEvent digit(QEvent::KeyPress, Qt::Key_3, Qt::NoModifier, "3");
    QApplication::sendEvent(points, &digit);
    QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(points, &enter);
    check(path.value()[2] == original[0] && path.value()[0] == original[1],
          "keyboard path reorder failed or changed point data");
  }
  for (int width : {240, 360, 520, 360, 240, 520})
  {
    scroll.resize(width, 720);
    flush();
    std::cout << "width=" << width << " path=" << points->width() << 'x'
              << points->height() << " brush=" << paint->width() << 'x'
              << paint->height() << '\n';
    check(points->width() == points->height(), "Path canvas is not square");
    check(paint->width() == paint->height(), "Brush canvas is not square");
  }
  const QPoint begin(paint->width() / 4, paint->height() / 2),
      end(paint->width() * 3 / 4, paint->height() / 2);
  mouse(paint, QEvent::MouseButtonPress, begin);
  mouse(paint, QEvent::MouseMove, end);
  mouse(paint, QEvent::MouseButtonRelease, end);
  check(paint->get_field_data()[128 * 256 + 128] > 0,
        "fast brush stroke has a gap");
  check(brush.value().shape == glm::ivec2(256),
        "display resize changed paint resolution");
  auto         buttons = rows[3]->findChildren<QPushButton *>();
  QPushButton *toggle = nullptr;
  for (auto *button : buttons)
    if (button->isCheckable()) toggle = button;
  toggle->click();
  check(!rows[3]->findChild<RangeBar *>()->isEnabled(),
        "range toggle did not disable range");
  toggle->click();
  check(range.value() == glm::vec2(.2f, .8f),
        "range toggle lost saved endpoints");
  if (argc > 1)
  {
    scroll.resize(440, 720);
    flush();
    QDir out(QString::fromLocal8Bit(argv[1]));
    out.mkpath(".");
    int i = 0;
    for (auto *row : rows)
      row->grab().save(out.filePath(QString("editor-%1.png").arg(i++)));
  }
  std::cout << "editor checks: failures=" << failures << '\n';
  return failures ? 1 : 0;
}
