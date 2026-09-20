// Offscreen snapshot harness for the ColorGradient row as Hesiod renders it
// (ottolink-dev/Hesiod#750): host palette and stylesheet, industrial and stock
// designs, inside a scroll area. Not part of the test suite; run with
// QT_QPA_PLATFORM=offscreen.
//
//   industrial_gradient_snap <out_dir> [darkstyle.css] [presets_dir]
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include <nlohmann/json.hpp>

#include "meta.hpp"
#include "meta_qt/container_widget.hpp"
#include "meta_qt/designs/industrial/industrial.hpp"
#include "meta_qt/designs/industrial/panel_chrome.hpp"
#include "meta_qt/designs/stock/stock.hpp"
#include "meta_qt/ui/design_registry.hpp"
#include "meta_qt/ui/theme.hpp"
#include "meta_qt/widgets/gradient_picker.hpp"

namespace
{

// Hesiod AppSettings::Colors defaults
const std::vector<std::pair<std::string, QColor>> kColors = {
    {"COLOR_BG_DEEP", QColor("#191919")},
    {"COLOR_BG_PRIMARY", QColor("#2B2B2B")},
    {"COLOR_BG_SECONDARY", QColor("#4B4B4B")},
    {"COLOR_TEXT_PRIMARY", QColor("#F4F4F5")},
    {"COLOR_TEXT_SECONDARY", QColor("#949495")},
    {"COLOR_TEXT_DISABLED", QColor("#3C3C3C")},
    {"COLOR_ACCENT", QColor("#5E81AC")},
    {"COLOR_BW_ACCENT", QColor("#FFFFFF")},
    {"COLOR_BORDER", QColor("#5B5B5B")},
    {"COLOR_HOVER", QColor("#8B8B8B")},
    {"COLOR_PRESSED", QColor("#ABABAB")},
    {"COLOR_SEPARATOR", QColor("#5B5B5B")}};

QColor color(const char *key)
{
  for (const auto &[k, c] : kColors)
    if (k == key) return c;
  return Qt::magenta;
}

void replace_all(std::string &s, const std::string &from, const std::string &to)
{
  for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos;
       pos += to.size())
    s.replace(pos, from.size(), to);
}

// darkstyle.css is a raw string literal for #include: strip its wrapper lines
QString hesiod_stylesheet(const std::string &path)
{
  std::ifstream in(path);
  if (!in) return {};
  std::stringstream ss;
  ss << in.rdbuf();
  std::string css = ss.str();
  const auto  first_nl = css.find('\n');
  const auto  last_paren = css.rfind(")\"\"");
  if (first_nl != std::string::npos && last_paren != std::string::npos &&
      last_paren > first_nl)
    css = css.substr(first_nl + 1, last_paren - first_nl - 1);
  for (const auto &[k, c] : kColors)
    replace_all(css, k, c.name().toStdString());
  return QString::fromStdString(css);
}

// Hesiod/data/color_gradients/*.json, named by file stem
std::vector<meta::Preset> load_presets(const std::string &dir)
{
  std::vector<meta::Preset> out;
  if (!std::filesystem::is_directory(dir)) return out;
  std::vector<std::filesystem::path> files;
  for (const auto &e : std::filesystem::directory_iterator(dir))
    if (e.is_regular_file() && e.path().extension() == ".json")
      files.push_back(e.path());
  std::sort(files.begin(), files.end());
  for (const auto &f : files)
  {
    std::ifstream  in(f);
    nlohmann::json j;
    in >> j;
    meta::Preset p;
    p.name = f.stem().string();
    for (const auto &s : j["value"])
    {
      meta::Stop stop;
      stop.position = s["position"].get<float>();
      for (size_t k = 0; k < 4; ++k)
        stop.color[k] = s["color"][k].get<float>();
      p.stops.push_back(stop);
    }
    out.push_back(std::move(p));
  }
  return out;
}

void describe_children(QWidget *root)
{
  auto vis = [](QWidget *w) { return w->isVisible() ? "visible" : "HIDDEN"; };
  for (auto *l : root->findChildren<QLabel *>())
    if (!l->text().isEmpty())
      std::cout << "  QLabel '" << l->text().toStdString() << "' " << vis(l)
                << " geom=" << l->geometry().x() << "," << l->geometry().y()
                << " " << l->width() << "x" << l->height()
                << " font=" << l->font().pointSize() << "pt\n";
  for (auto *c : root->findChildren<QComboBox *>())
    std::cout << "  QComboBox text='" << c->currentText().toStdString() << "' "
              << vis(c) << " geom=" << c->geometry().x() << ","
              << c->geometry().y() << " " << c->width() << "x" << c->height()
              << "\n";
  for (auto *b : root->findChildren<QToolButton *>())
    std::cout << "  QToolButton '" << b->text().toStdString() << "' " << vis(b)
              << " geom=" << b->geometry().x() << "," << b->geometry().y()
              << " " << b->width() << "x" << b->height() << "\n";
  int n = 0;
  for (auto *b : root->findChildren<QPushButton *>())
    if (b->property("preset_name").isValid()) ++n;
  std::cout << "  preset buttons: " << n << "\n";
  if (auto *p = root->findChild<meta::qt::GradientPicker *>())
  {
    std::cout << "  GradientPicker geom=" << p->geometry().x() << ","
              << p->geometry().y() << " " << p->width() << "x" << p->height()
              << " sizeHint=" << static_cast<QWidget *>(p)->sizeHint().width()
              << "x" << static_cast<QWidget *>(p)->sizeHint().height()
              << " minHint="
              << static_cast<QWidget *>(p)->minimumSizeHint().height() << "\n";
    if (auto *s = p->findChild<QScrollArea *>())
      std::cout << "  preset scroll viewport=" << s->viewport()->width() << "x"
                << s->viewport()->height()
                << " content=" << (s->widget() ? s->widget()->height() : -1)
                << "\n";
  }
}

} // namespace

int main(int argc, char **argv)
{
  QApplication      app(argc, argv);
  const QString     out = argc > 1 ? argv[1] : ".";
  const std::string css = argc > 2 ? argv[2] : "darkstyle.css";
  const std::string presets_dir = argc > 3 ? argv[3] : "color_gradients";
  QDir().mkpath(out);

  // host palette + stylesheet, as apply_global_style does
  {
    QPalette pal = app.palette();
    pal.setColor(QPalette::Window, color("COLOR_BG_PRIMARY"));
    pal.setColor(QPalette::WindowText, color("COLOR_TEXT_PRIMARY"));
    pal.setColor(QPalette::Base, color("COLOR_BG_SECONDARY"));
    pal.setColor(QPalette::AlternateBase, color("COLOR_BG_PRIMARY"));
    pal.setColor(QPalette::Text, color("COLOR_TEXT_PRIMARY"));
    pal.setColor(QPalette::PlaceholderText, color("COLOR_TEXT_SECONDARY"));
    pal.setColor(QPalette::Button, color("COLOR_BG_SECONDARY"));
    pal.setColor(QPalette::ButtonText, color("COLOR_TEXT_PRIMARY"));
    pal.setColor(QPalette::BrightText, color("COLOR_TEXT_PRIMARY"));
    pal.setColor(QPalette::Highlight, color("COLOR_ACCENT"));
    pal.setColor(QPalette::HighlightedText, color("COLOR_TEXT_PRIMARY"));
    pal.setColor(QPalette::ToolTipBase, color("COLOR_BG_DEEP"));
    pal.setColor(QPalette::ToolTipText, color("COLOR_TEXT_PRIMARY"));
    pal.setColor(QPalette::Mid, color("COLOR_BORDER"));
    pal.setColor(QPalette::Dark, color("COLOR_BG_DEEP"));
    pal.setColor(QPalette::Light, color("COLOR_HOVER"));
    for (const auto role : {QPalette::Text,
                            QPalette::WindowText,
                            QPalette::ButtonText,
                            QPalette::HighlightedText})
      pal.setColor(QPalette::Disabled, role, color("COLOR_TEXT_DISABLED"));
    app.setPalette(pal);
    const QString sheet = hesiod_stylesheet(css);
    std::cout << "stylesheet chars: " << sheet.size() << "\n";
    app.setStyleSheet(sheet);
  }

  // designs, as properties_panel_design does
  meta::qt::industrial::register_design();
  meta::qt::stock::register_design();
  auto &registry = meta::qt::DesignRegistry::instance();
  registry.set_theme(meta::qt::industrial::kDesignName, "palette");
  const meta::qt::Theme &theme = registry.theme(
      meta::qt::industrial::kDesignName);

  meta::GradientLibrary &lib = meta::GradientLibrary::instance();
  lib.set_path(std::filesystem::temp_directory_path() /
               "meta_industrial_gradient_snap" / "gradients.json");
  lib.clear();

  const auto presets = load_presets(presets_dir);
  std::cout << "presets loaded: " << presets.size() << "\n";

  for (const std::string design : {"industrial", "stock"})
  {
    const bool own_chrome = design == "industrial";

    meta::AttributeContainer c;
    auto &g = meta::presets::color_gradient(c, "gradient", "gradient");
    g.metadata().add(meta::keys::ui::presets, meta::GradientPresets{presets});
    meta::presets::checkbox(c, "reverse_colormap", "reverse_colormap", false);
    meta::presets::checkbox(c, "reverse_alpha", "reverse_alpha", false);
    meta::presets::checkbox(c, "clamp_alpha", "clamp_alpha", true);

    meta::qt::ContainerRenderOptions opt;
    opt.design = design;
    opt.row_context.theme = &theme;
    opt.category_policy = meta::qt::CategoryPolicy::CP_MERGED;
    opt.root_category_name = own_chrome ? "Parameters" : "";

    for (int width : {360, 480})
    {
      QWidget host;
      auto   *hl = new QVBoxLayout(&host);
      hl->setContentsMargins(0, 0, 0, 0);

      auto *container = new QWidget();
      auto *al = new QVBoxLayout(container);
      al->setAlignment(Qt::AlignTop);
      if (own_chrome) al->setContentsMargins(0, 0, 0, 0);
      al->setSpacing(2);

      auto *w = meta::qt::render(c, opt, container);
      if (w && w->layout()) w->layout()->setAlignment(Qt::AlignTop);
      al->addWidget(w);

      auto *scroll = new QScrollArea();
      scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
      if (own_chrome)
      {
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        scroll->setStyleSheet(
            meta::qt::industrial::scrollbar_stylesheet(theme));
      }
      scroll->setWidget(container);
      scroll->setWidgetResizable(true);
      scroll->setFrameShape(QFrame::NoFrame);
      hl->addWidget(scroll);

      host.resize(width, 900);
      host.show();
      QApplication::processEvents();
      QApplication::processEvents();

      const QString name = QString("%1/%2_%3.png")
                               .arg(out, QString::fromStdString(design))
                               .arg(width);
      host.grab().save(name);
      std::cout << "== " << design << " @" << width << " -> "
                << name.toStdString() << "\n";
      describe_children(&host);
    }
  }
  return 0;
}
