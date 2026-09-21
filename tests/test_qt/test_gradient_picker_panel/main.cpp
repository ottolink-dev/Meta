// GradientPicker as a panel row (ottolink-dev/Hesiod#750). Run with
// QT_QPA_PLATFORM=offscreen; exits non-zero on failure.
#include <filesystem>
#include <iostream>
#include <vector>

#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include "meta/core/attribute_container.hpp"
#include "meta/ext/color_gradient/color_gradient.hpp"
#include "meta/ext/color_gradient/gradient_library.hpp"
#include "meta/metadata/keys.hpp"
#include "meta/presets/color_gradient.hpp"
#include "meta_qt/container_widget.hpp"
#include "meta_qt/designs/industrial/industrial.hpp"
#include "meta_qt/ui/design_registry.hpp"
#include "meta_qt/ui/theme.hpp"
#include "meta_qt/widgets/gradient_picker.hpp"

using namespace meta::qt;
using meta::Preset;
using meta::Stop;

namespace
{

int  failures = 0;
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

Preset flat(const std::string &name, float grey)
{
  return {name,
          {{0.f, {grey, grey, grey, 1.f}}, {1.f, {grey, grey, grey, 1.f}}}};
}

std::vector<Preset> hex_presets(int n)
{
  std::vector<Preset> out;
  for (int i = 0; i < n; ++i)
  {
    char name[8];
    std::snprintf(name, sizeof(name), "%06x", 0x100000 + i * 7919);
    out.push_back(flat(name, 0.9f));
  }
  return out;
}

// Paints its own surface like industrial::Section, out of reach of a `*` rule
class Card : public QWidget
{
public:
  explicit Card(QColor colour) : colour_(colour) {}

protected:
  void paintEvent(QPaintEvent *) override
  {
    QPainter p(this);
    p.fillRect(rect(), colour_);
  }

private:
  QColor colour_;
};

QPushButton *first_swatch(QWidget &picker)
{
  for (auto *b : picker.findChildren<QPushButton *>())
    if (b->property("preset_name").isValid()) return b;
  return nullptr;
}

} // namespace

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  // Isolated library so nothing here touches a real per-user file
  meta::GradientLibrary &lib = meta::GradientLibrary::instance();
  lib.set_path(std::filesystem::temp_directory_path() /
               "meta_test_gradient_picker_panel" / "gradients.json");
  lib.clear();

  std::vector<Stop> stops = {{0.f, {0.f, 0.f, 0.f, 1.f}},
                             {1.f, {1.f, 1.f, 1.f, 1.f}}};

  // --- 1. a six-hex-digit preset name is still drawn on its swatch
  {
    GradientPicker picker(stops, {flat("051c4a", 0.9f)});
    picker.resize(360, 400);
    picker.show();
    flush();

    auto *btn = first_swatch(picker);
    check(btn != nullptr, "no swatch button for the preset");
    if (btn)
    {
      const QImage img = btn->icon().pixmap(btn->iconSize()).toImage();
      check(!img.isNull(), "swatch icon is empty");
      if (!img.isNull())
      {
        const int    x = img.width() / 2;
        const QColor top = img.pixelColor(x, 3);
        const QColor bottom = img.pixelColor(x, img.height() - 4);
        check(bottom.lightness() < top.lightness() - 40,
              "hex-named swatch has no name band along its bottom edge");
      }
    }
  }

  // --- 2. the sort control carries a visible "Sort" caption
  {
    GradientPicker picker(stops, hex_presets(4));
    picker.resize(360, 400);
    picker.show();
    flush();

    bool captioned = false;
    for (auto *label : picker.findChildren<QLabel *>())
      if (label->text().compare("Sort", Qt::CaseInsensitive) == 0 &&
          label->isVisibleTo(&picker))
        captioned = true;
    check(captioned, "sort combo has no visible Sort caption");
  }

  // --- 3. the preferred height follows the width and shows at least six rows
  {
    GradientPicker picker(stops, hex_presets(40));
    picker.resize(360, 200);
    picker.show();
    flush();

    QWidget &w = picker;
    check(w.hasHeightForWidth(), "picker does not report height-for-width");

    const int h = w.heightForWidth(360);
    picker.resize(360, h);
    flush();

    auto *btn = first_swatch(picker);
    auto *scroll = picker.findChild<QScrollArea *>();
    check(btn && scroll, "missing swatch or scroll area");
    if (btn && scroll)
    {
      const int row = btn->height();
      check(scroll->viewport()->height() >= 6 * row,
            "at its preferred height the grid shows fewer than six rows");
    }

    GradientPicker few(stops, hex_presets(3));
    few.resize(360, 200);
    few.show();
    flush();
    const int h_few = static_cast<QWidget &>(few).heightForWidth(360);
    check(h_few < h,
          "three presets ask for as much height as forty: the preferred "
          "height does not follow the preset count");
  }

  // --- 4. industrial editor: a host `*` background rule paints no slab
  {
    app.setStyleSheet("* { background-color: #2b2b2b; }");
    industrial::register_design();
    auto        &registry = DesignRegistry::instance();
    const Theme &theme = registry.theme(industrial::kDesignName);

    meta::Attribute<meta::ColorGradient> attr("gradient",
                                              meta::ColorGradient{});
    attr.metadata().add(meta::keys::ui::label, std::string("gradient"));
    attr.metadata().add(meta::keys::ui::presets,
                        meta::GradientPresets{hex_presets(6)});

    RowContext ctx;
    ctx.theme = &theme;

    Card  card(theme.section_surface);
    auto *layout = new QVBoxLayout(&card);
    layout->setContentsMargins(12, 12, 12, 12);

    MetaWidget *editor = registry.render(&attr, industrial::kDesignName, ctx);
    check(editor != nullptr, "industrial design did not render the gradient");
    if (editor)
    {
      layout->addWidget(editor);
      card.resize(420, 500);
      card.show();
      flush();

      QToolButton *save = nullptr;
      for (auto *b : editor->findChildren<QToolButton *>())
        if (b->text().startsWith("Save")) save = b;
      check(save != nullptr, "no Save preset button");
      if (save)
      {
        const QImage img = card.grab().toImage();
        const QPoint p = save->mapTo(
            &card,
            QPoint(save->width() + 12, save->height() / 2));
        const QColor got = img.pixelColor(p);
        check(got.rgb() == theme.section_surface.rgb(),
              "toolbar row background is not the section card colour");
        if (got.rgb() != theme.section_surface.rgb())
          std::cerr << "  got " << got.name().toStdString() << " expected "
                    << theme.section_surface.name().toStdString() << " at "
                    << p.x() << "," << p.y() << '\n';
      }
    }
  }

  // --- 5. inside an industrial section the picker gets its preferred height
  {
    industrial::register_design();
    auto        &registry = DesignRegistry::instance();
    const Theme &theme = registry.theme(industrial::kDesignName);

    meta::AttributeContainer c;
    auto &g = meta::presets::color_gradient(c, "gradient", "gradient");
    g.metadata().add(meta::keys::ui::presets,
                     meta::GradientPresets{hex_presets(40)});

    ContainerRenderOptions opt;
    opt.design = industrial::kDesignName;
    opt.row_context.theme = &theme;
    opt.category_policy = CategoryPolicy::CP_MERGED;
    opt.root_category_name = "Parameters";

    QWidget host;
    auto   *layout = new QVBoxLayout(&host);
    layout->setContentsMargins(0, 0, 0, 0);
    MetaWidget *panel = render(c, opt, &host);
    check(panel != nullptr, "industrial panel did not render");
    if (panel)
    {
      layout->addWidget(panel);
      layout->addStretch();
      host.resize(360, 900);
      host.show();
      flush();

      auto *picker = host.findChild<GradientPicker *>();
      check(picker != nullptr, "no picker in the industrial panel");
      if (picker)
      {
        QWidget  &w = *picker;
        const int want = w.heightForWidth(w.width());
        check(w.height() > 0, "picker collapsed to zero height in a section");
        check(w.height() == want,
              "picker in a section is not at its preferred height");
        if (w.height() != want)
          std::cerr << "  height " << w.height() << " preferred " << want
                    << '\n';
        auto *btn = first_swatch(*picker);
        auto *scroll = picker->findChild<QScrollArea *>();
        if (btn && scroll)
          check(scroll->viewport()->height() >= 6 * btn->height(),
                "section shows fewer than six swatch rows");
      }
    }
  }

  std::cout << "gradient picker panel checks: failures=" << failures << '\n';
  return failures ? 1 : 0;
}
