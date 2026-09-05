/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>

#include <QColorDialog>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QToolButton>
#include <QVBoxLayout>

#include "meta/ext/color_gradient/gradient_library.hpp"
#include "meta/ext/color_gradient/gradient_metrics.hpp"
#include "meta/logger.hpp"
#include "meta_qt/widgets/gradient_picker.hpp"

namespace meta::qt {

// ---------------------------------------------------------------------------
// Colour conversion helpers
// ---------------------------------------------------------------------------

static QColor to_qcolor(const std::array<float, 4> &c) {
  return QColor(int(std::clamp(c[0], 0.f, 1.f) * 255.f),
                int(std::clamp(c[1], 0.f, 1.f) * 255.f),
                int(std::clamp(c[2], 0.f, 1.f) * 255.f),
                int(std::clamp(c[3], 0.f, 1.f) * 255.f));
}

static std::array<float, 4> from_qcolor(const QColor &c) {
  return {float(c.redF()), float(c.greenF()), float(c.blueF()),
          float(c.alphaF())};
}

// ---------------------------------------------------------------------------
// Library helpers
// ---------------------------------------------------------------------------

// Gives the process-wide GradientLibrary a per-user file on first use, unless
// the host already chose one (set_path() before any picker exists).
static void ensure_gradient_library() {
  GradientLibrary &lib = GradientLibrary::instance();
  if (!lib.path().empty())
    return;

  QString dir =
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  if (dir.isEmpty())
    dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

  if (dir.isEmpty()) {
    static bool warned = false;
    if (!warned)
      Logger::log()->warn("GradientPicker: no writable config location, the "
                          "gradient library will not persist");
    warned = true;
    return;
  }

  lib.set_path(std::filesystem::path(dir.toStdString()) / "gradients.json");
  lib.load();

  Logger::log()->trace("GradientPicker: gradient library at '{}'",
                       lib.path().string());
}

static QString gradient_file_filter() {
  return QObject::tr("Gradient files (*.json);;All files (*)");
}

static void draw_star(QPainter &p, const QPointF &center, qreal radius) {
  QPolygonF star;
  for (int i = 0; i < 10; ++i) {
    const qreal r = (i % 2 == 0) ? radius : radius * 0.45;
    const qreal a = -std::numbers::pi / 2.0 + i * std::numbers::pi / 5.0;
    star << QPointF(center.x() + r * std::cos(a), center.y() + r * std::sin(a));
  }
  p.setPen(QPen(QColor(40, 40, 40), 1));
  p.setBrush(QColor(255, 200, 40));
  p.drawPolygon(star);
}

// ---------------------------------------------------------------------------
// GradientBarWidget
// ---------------------------------------------------------------------------

GradientBarWidget::GradientBarWidget(std::vector<Stop> &stops, QWidget *parent)
    : QWidget(parent), stops_(stops) {
  setFixedHeight(TOTAL_H);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void GradientBarWidget::sort_stops() {
  std::sort(stops_.begin(), stops_.end(), [](const Stop &a, const Stop &b) {
    return a.position < b.position;
  });
}

void GradientBarWidget::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  const QRectF br = bar_rect();
  const QPalette &pal = palette();

  // Gradient bar
  {
    QLinearGradient grad(br.topLeft(), br.topRight());
    for (const auto &s : stops_)
      grad.setColorAt(double(s.position), to_qcolor(s.color));
    p.setBrush(grad);
    p.setPen(QPen(pal.color(QPalette::Mid), 1));
    p.drawRoundedRect(br, RADIUS, RADIUS);
  }

  // Stop handles
  for (int i = 0; i < static_cast<int>(stops_.size()); ++i) {
    const QRectF r = stop_rect(stops_[i]);
    const bool sel = (i == selected_idx_);

    // Small triangle pointing up from bar bottom to handle
    const float cx = float(r.center().x());
    const float ty = float(br.bottom());
    QPolygonF tri;
    tri << QPointF(cx - 4, ty + 8) << QPointF(cx + 4, ty + 8)
        << QPointF(cx, ty + 1);
    p.setPen(Qt::NoPen);
    p.setBrush(pal.color(sel ? QPalette::Highlight : QPalette::Button));
    p.drawPolygon(tri);

    // Colour disc
    p.setBrush(to_qcolor(stops_[i].color));
    p.setPen(
        QPen(sel ? pal.color(QPalette::Highlight) : pal.color(QPalette::Dark),
             sel ? 2 : 1));
    p.drawEllipse(r);
  }
}

void GradientBarWidget::mouseDoubleClickEvent(QMouseEvent *e) {
  const int idx = hit_test(e->pos());

  if (idx >= 0) {
    // Edit existing stop colour
    const QColor picked = QColorDialog::getColor(
        to_qcolor(stops_[idx].color), this, QString(),
        QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);

    if (picked.isValid()) {
      stops_[idx].color = from_qcolor(picked);
      update();
      Q_EMIT value_changed();
      Q_EMIT edit_ended();
    }
  } else if (bar_rect().contains(QPointF(e->pos()))) {
    // Add new stop
    const double pos = std::clamp(
        (e->pos().x() - bar_rect().left()) / bar_rect().width(), 0.0, 1.0);

    constexpr double eps = 1e-3;
    const bool too_close =
        std::any_of(stops_.begin(), stops_.end(), [&](const Stop &s) {
          return std::abs(double(s.position) - pos) < eps;
        });

    if (!too_close) {
      stops_.push_back({float(pos), {1.f, 1.f, 1.f, 1.f}});
      sort_stops();
      auto it =
          std::find_if(stops_.begin(), stops_.end(), [pos](const Stop &s) {
            return s.position == float(pos);
          });
      if (it != stops_.end())
        selected_idx_ = static_cast<int>(std::distance(stops_.begin(), it));
      update();
      Q_EMIT value_changed();
      Q_EMIT edit_ended();
    }
  }
}

void GradientBarWidget::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    selected_idx_ = hit_test(e->pos());
    dragging_ = selected_idx_ >= 0;
    update();
  }
}

void GradientBarWidget::mouseMoveEvent(QMouseEvent *e) {
  if (!dragging_ || selected_idx_ < 0 ||
      selected_idx_ >= static_cast<int>(stops_.size()))
    return;

  const QRectF br = bar_rect();
  if (br.width() <= 0)
    return;

  const double pos =
      std::clamp((e->pos().x() - br.left()) / br.width(), 0.0, 1.0);

  stops_[selected_idx_].position = float(pos);

  // Maintain sorted order while keeping selected_idx_ tracking the moved stop
  while (selected_idx_ > 0 &&
         stops_[selected_idx_].position < stops_[selected_idx_ - 1].position) {
    std::swap(stops_[selected_idx_], stops_[selected_idx_ - 1]);
    --selected_idx_;
  }
  while (selected_idx_ + 1 < static_cast<int>(stops_.size()) &&
         stops_[selected_idx_].position > stops_[selected_idx_ + 1].position) {
    std::swap(stops_[selected_idx_], stops_[selected_idx_ + 1]);
    ++selected_idx_;
  }

  update();
  Q_EMIT value_changed();
}

void GradientBarWidget::mouseReleaseEvent(QMouseEvent *) {
  if (dragging_) {
    dragging_ = false;
    Q_EMIT edit_ended();
  }
}

void GradientBarWidget::contextMenuEvent(QContextMenuEvent *e) {
  const int idx = hit_test(e->pos());
  if (idx < 0)
    return;

  // Only offer removal when at least 3 stops (keep minimum 2).
  if (static_cast<int>(stops_.size()) <= 2)
    return;

  QMenu menu(this);
  QAction *rm = menu.addAction(QObject::tr("Remove stop"));

  if (menu.exec(e->globalPos()) == rm) {
    stops_.erase(stops_.begin() + idx);

    if (selected_idx_ == idx)
      selected_idx_ = -1;
    else if (selected_idx_ > idx)
      --selected_idx_;

    update();
    Q_EMIT value_changed();
    Q_EMIT edit_ended();
  }
}

QRectF GradientBarWidget::bar_rect() const {
  return QRectF(PAD, PAD, width() - 2 * PAD, BAR_H);
}

QRectF GradientBarWidget::stop_rect(const Stop &s) const {
  const QRectF br = bar_rect();
  const double cx = br.left() + double(s.position) * br.width();
  const double cy = br.bottom() + 3 + STOP_R;
  return QRectF(cx - STOP_R, cy - STOP_R, STOP_R * 2, STOP_R * 2);
}

int GradientBarWidget::hit_test(const QPoint &pos) const {
  for (int i = static_cast<int>(stops_.size()) - 1; i >= 0; --i)
    if (stop_rect(stops_[i]).adjusted(-2, -2, 2, 2).contains(QPointF(pos)))
      return i;
  return -1;
}

// ---------------------------------------------------------------------------
// PresetGridWidget: responsive grid that wraps swatches based on width
// ---------------------------------------------------------------------------

class PresetGridWidget : public QWidget {
public:
  explicit PresetGridWidget(int swatch_w, int swatch_h, int spacing = 4,
                            QWidget *parent = nullptr)
      : QWidget(parent), swatch_w_(swatch_w), swatch_h_(swatch_h),
        spacing_(spacing) {
    // Tell the layout system that our height depends on our width, so
    // QVBoxLayout / QScrollArea can query heightForWidth() instead of
    // relying on a stale, width-independent sizeHint().
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    setSizePolicy(sp);

    init_layout();
  }

  void set_buttons(const std::vector<QPushButton *> &buttons) {
    buttons_ = buttons;
    current_cols_ = -1;
    reflow(width());
  }

  void reflow(int avail_w) {
    if (buttons_.empty())
      return;

    const int cols = compute_cols(avail_w);
    if (cols == current_cols_) {
      const int h = heightForWidth(avail_w);
      setMinimumHeight(h);
      return;
    }
    current_cols_ = cols;

    // Destroy old layout to fully reset QGridLayout internal column
    // dimensions
    delete grid_;
    init_layout();

    for (size_t i = 0; i < buttons_.size(); ++i)
      grid_->addWidget(buttons_[i], static_cast<int>(i / cols),
                       static_cast<int>(i % cols));

    const int h = heightForWidth(avail_w);
    setMinimumHeight(h);
    updateGeometry();
  }

  bool hasHeightForWidth() const override { return true; }

  int heightForWidth(int w) const override {
    if (buttons_.empty())
      return 0;
    const int cols = compute_cols(w);
    const int rows = (static_cast<int>(buttons_.size()) + cols - 1) / cols;
    return 4 + rows * swatch_h_ + (rows - 1) * spacing_;
  }

  QSize sizeHint() const override {
    if (buttons_.empty())
      return QSize(0, 0);
    const int cols = current_cols_ > 0 ? current_cols_ : 1;
    const int w = 4 + cols * swatch_w_ + (cols - 1) * spacing_;
    return QSize(w, heightForWidth(width() > 0 ? width() : w));
  }

  QSize minimumSizeHint() const override {
    if (buttons_.empty())
      return QSize(0, 0);
    return QSize(swatch_w_ + 4,
                 heightForWidth(width() > 0 ? width() : swatch_w_ + 4));
  }

protected:
  void resizeEvent(QResizeEvent *event) override {
    QWidget::resizeEvent(event);
    reflow(event->size().width());
  }

private:
  int compute_cols(int avail_w) const {
    const int margins_w = 4;
    return std::max(1,
                    (avail_w - margins_w + spacing_) / (swatch_w_ + spacing_));
  }

  void init_layout() {
    grid_ = new QGridLayout(this);
    grid_->setContentsMargins(2, 2, 2, 2);
    grid_->setSpacing(spacing_);
    grid_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    grid_->setSizeConstraint(QLayout::SetNoConstraint);
  }

  int swatch_w_;
  int swatch_h_;
  int spacing_;
  int current_cols_ = -1;
  QGridLayout *grid_ = nullptr;
  std::vector<QPushButton *> buttons_;
};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

GradientPicker::GradientPicker(std::vector<Stop> &stops,
                               const std::vector<Preset> &presets,
                               QWidget *parent)
    : QWidget(parent), stops_(stops), presets_(presets) {
  ensure_gradient_library();

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  auto *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(0, 0, 0, 0);
  main_layout->setSpacing(4);

  // Gradient bar pinned at the top (fixed height, never scrolls)
  bar_widget_ = new GradientBarWidget(stops_, this);
  main_layout->addWidget(bar_widget_);

  // Library toolbar
  main_layout->addWidget(build_toolbar());

  // Preset selection grid inside scroll area
  scroll_area_ = new QScrollArea(this);
  scroll_area_->setFrameShape(QFrame::NoFrame);
  scroll_area_->setWidgetResizable(true);
  scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  scroll_area_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  preset_grid_ = new PresetGridWidget(SWATCH_W, SWATCH_H, 4, scroll_area_);
  scroll_area_->setWidget(preset_grid_);

  if (scroll_area_->viewport())
    scroll_area_->viewport()->installEventFilter(this);

  main_layout->addWidget(scroll_area_, 1);

  connect(bar_widget_, &GradientBarWidget::value_changed, this,
          &GradientPicker::value_changed);
  connect(bar_widget_, &GradientBarWidget::edit_ended, this,
          &GradientPicker::edit_ended);

  // Any picker (or the host) editing the library refreshes this grid
  library_connection_ = GradientLibrary::instance().changed.subscribe(
      [this]() { schedule_rebuild(); });

  rebuild_preset_grid();
}

// ---------------------------------------------------------------------------
// Toolbar
// ---------------------------------------------------------------------------

QWidget *GradientPicker::build_toolbar() {
  auto *bar = new QWidget(this);
  auto *layout = new QHBoxLayout(bar);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(4);

  const auto make_button = [bar](const QString &text, const QString &tip) {
    auto *button = new QToolButton(bar);
    button->setText(text);
    button->setToolTip(tip);
    button->setAutoRaise(true);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setFixedHeight(TOOLBAR_H);
    button->setCursor(Qt::PointingHandCursor);
    return button;
  };

  save_button_ = make_button(tr("Save..."),
                             tr("Save the current gradient to your library"));
  import_button_ =
      make_button(tr("Import..."), tr("Import gradients from JSON files"));
  export_button_ =
      make_button(tr("Export..."), tr("Export your library to a JSON file"));

  connect(save_button_, &QToolButton::clicked, this,
          &GradientPicker::on_save_clicked);
  connect(import_button_, &QToolButton::clicked, this,
          &GradientPicker::on_import_clicked);
  connect(export_button_, &QToolButton::clicked, this,
          &GradientPicker::on_export_clicked);

  sort_combo_ = new QComboBox(bar);
  sort_combo_->setToolTip(tr("Preset ordering (favorites always come first)"));
  sort_combo_->setFixedHeight(TOOLBAR_H);
  // item order == meta::GradientSort
  sort_combo_->addItems(
      {tr("Default"), tr("Name"), tr("Luminance"), tr("Hue")});

  // `activated` fires for user picks only, so syncing the index from the
  // library in rebuild_preset_grid() cannot loop back.
  connect(sort_combo_, QOverload<int>::of(&QComboBox::activated), this,
          [](int index) {
            GradientLibrary::instance().set_sort(
                static_cast<GradientSort>(index));
          });

  layout->addWidget(save_button_);
  layout->addWidget(import_button_);
  layout->addWidget(export_button_);
  layout->addStretch(1);
  layout->addWidget(new QLabel(tr("Sort"), bar));
  layout->addWidget(sort_combo_);

  return bar;
}

// ---------------------------------------------------------------------------
// Preset grid
// ---------------------------------------------------------------------------

void GradientPicker::set_presets(const std::vector<Preset> &presets) {
  presets_ = presets;
  rebuild_preset_grid();
}

void GradientPicker::update_bar() {
  if (bar_widget_)
    bar_widget_->update();
}

void GradientPicker::schedule_rebuild() {
  // Deferred: the notification may come from a slot of a swatch button that
  // the rebuild is about to delete, and bursts (imports) coalesce.
  if (rebuild_pending_)
    return;
  rebuild_pending_ = true;

  QMetaObject::invokeMethod(
      this,
      [this]() {
        rebuild_pending_ = false;
        rebuild_preset_grid();
      },
      Qt::QueuedConnection);
}

void GradientPicker::rebuild_entries() {
  const GradientLibrary &lib = GradientLibrary::instance();
  const GradientSort sort = lib.sort();

  struct Keyed {
    Entry entry;
    bool favorite;
    float key;
    std::string lower_name;
    std::size_t index;
  };

  std::vector<Keyed> keyed;
  keyed.reserve(presets_.size() + lib.presets().size());

  const auto push = [&](const Preset &preset, bool user) {
    Keyed k{Entry{preset, user}, lib.is_favorite(preset.name), 0.f, preset.name,
            keyed.size()};

    std::transform(k.lower_name.begin(), k.lower_name.end(),
                   k.lower_name.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });

    if (sort == GradientSort::Luminance)
      k.key = gradient_luminance(preset.stops);
    else if (sort == GradientSort::Hue) {
      const float hue = gradient_hue(preset.stops);
      k.key = hue < 0.f ? 1e6f : hue; // achromatic last
    }

    keyed.push_back(std::move(k));
  };

  for (const auto &preset : presets_)
    push(preset, false);
  for (const auto &preset : lib.presets())
    push(preset, true);

  std::stable_sort(keyed.begin(), keyed.end(),
                   [sort](const Keyed &a, const Keyed &b) {
                     if (a.favorite != b.favorite)
                       return a.favorite;

                     switch (sort) {
                     case GradientSort::Name:
                       return a.lower_name < b.lower_name;
                     case GradientSort::Luminance:
                     case GradientSort::Hue:
                       if (a.key != b.key)
                         return a.key < b.key;
                       return a.lower_name < b.lower_name;
                     case GradientSort::Default:
                     default:
                       return a.index < b.index;
                     }
                   });

  entries_.clear();
  entries_.reserve(keyed.size());
  for (auto &k : keyed)
    entries_.push_back(std::move(k.entry));
}

QPixmap GradientPicker::make_swatch(const Entry &entry, bool favorite) const {
  QPixmap pix(SWATCH_W, SWATCH_H);
  QPainter pp(&pix);
  pp.setRenderHint(QPainter::Antialiasing);

  QLinearGradient grad(0, 0, pix.width(), 0);
  for (const auto &s : entry.preset.stops)
    grad.setColorAt(double(s.position), to_qcolor(s.color));
  pp.fillRect(pix.rect(), grad);

  // Name overlay
  pp.setPen(Qt::white);
  pp.setFont(QFont(pp.font().family(), 7));
  pp.drawText(pix.rect().adjusted(2, 0, -2, 0),
              Qt::AlignBottom | Qt::AlignHCenter,
              QString::fromStdString(entry.preset.name));

  // Favourite star (top-left) and library marker (top-right)
  if (favorite)
    draw_star(pp, QPointF(8, 8), 5.5);

  if (entry.user) {
    pp.setPen(QPen(QColor(40, 40, 40), 1));
    pp.setBrush(Qt::white);
    pp.drawEllipse(QPointF(pix.width() - 7, 7), 3, 3);
  }

  // Border
  pp.setPen(QPen(QColor(80, 80, 80), 1));
  pp.setBrush(Qt::NoBrush);
  pp.drawRoundedRect(pix.rect().adjusted(0, 0, -1, -1),
                     GradientBarWidget::RADIUS, GradientBarWidget::RADIUS);

  return pix;
}

void GradientPicker::rebuild_preset_grid() {
  const GradientLibrary &lib = GradientLibrary::instance();

  rebuild_entries();

  if (sort_combo_) {
    const QSignalBlocker blocker(sort_combo_);
    sort_combo_->setCurrentIndex(static_cast<int>(lib.sort()));
  }
  if (export_button_)
    export_button_->setEnabled(!lib.presets().empty());

  scroll_area_->setVisible(!entries_.empty());

  // Delete all existing child buttons inside preset_grid_
  qDeleteAll(preset_grid_->findChildren<QPushButton *>(
      QString(), Qt::FindDirectChildrenOnly));

  std::vector<QPushButton *> buttons;
  buttons.reserve(entries_.size());

  for (std::size_t i = 0; i < entries_.size(); ++i) {
    const Entry &entry = entries_[i];
    const bool favorite = lib.is_favorite(entry.preset.name);
    const QString name = QString::fromStdString(entry.preset.name);

    auto *btn = new QPushButton(preset_grid_);
    btn->setFixedSize(SWATCH_W, SWATCH_H);
    btn->setFlat(true);
    btn->setIcon(QIcon(make_swatch(entry, favorite)));
    btn->setIconSize(QSize(SWATCH_W, SWATCH_H));
    btn->setToolTip(
        QString("%1\n%2, %3 %4")
            .arg(name, entry.user ? tr("Library preset") : tr("Preset"))
            .arg(int(entry.preset.stops.size()))
            .arg(tr("stops")));
    btn->setCursor(Qt::PointingHandCursor);
    btn->setProperty("preset_name", name);
    btn->setProperty("preset_user", entry.user);
    btn->setContextMenuPolicy(Qt::CustomContextMenu);

    const std::vector<Stop> preset_stops = entry.preset.stops;
    connect(btn, &QPushButton::clicked, this,
            [this, preset_stops]() { apply_stops(preset_stops); });

    connect(btn, &QPushButton::customContextMenuRequested, this,
            [this, btn, i](const QPoint &pos) {
              if (i < entries_.size())
                show_entry_menu(entries_[i], btn->mapToGlobal(pos));
            });

    buttons.push_back(btn);
  }

  preset_grid_->set_buttons(buttons);
  if (scroll_area_ && scroll_area_->viewport())
    preset_grid_->reflow(scroll_area_->viewport()->width());

  updateGeometry();
}

void GradientPicker::apply_stops(const std::vector<Stop> &stops) {
  stops_ = stops;
  if (bar_widget_) {
    bar_widget_->sort_stops();
    bar_widget_->update();
  }
  Q_EMIT value_changed();
  Q_EMIT edit_ended();
}

std::vector<std::string> GradientPicker::host_names() const {
  std::vector<std::string> names;
  names.reserve(presets_.size());
  for (const auto &preset : presets_)
    names.push_back(preset.name);
  return names;
}

std::vector<std::string> GradientPicker::entry_names() const {
  std::vector<std::string> names;
  names.reserve(entries_.size());
  for (const auto &entry : entries_)
    names.push_back(entry.preset.name);
  return names;
}

// ---------------------------------------------------------------------------
// Library actions
// ---------------------------------------------------------------------------

std::string GradientPicker::save_current_as_preset(const std::string &name) {
  GradientLibrary &lib = GradientLibrary::instance();

  Preset preset;
  preset.name = lib.unique_name(name, host_names());
  preset.stops = stops_;

  return lib.add(std::move(preset));
}

void GradientPicker::on_save_clicked() {
  GradientLibrary &lib = GradientLibrary::instance();

  bool ok = false;
  const QString text = QInputDialog::getText(
      this, tr("Save gradient"), tr("Preset name:"), QLineEdit::Normal,
      QString::fromStdString(lib.unique_name("Gradient", host_names())), &ok);

  if (!ok || text.trimmed().isEmpty())
    return;

  save_current_as_preset(text.trimmed().toStdString());
}

void GradientPicker::on_import_clicked() {
  const QStringList files = QFileDialog::getOpenFileNames(
      this, tr("Import gradients"), QString(), gradient_file_filter());
  if (files.isEmpty())
    return;

  GradientLibrary &lib = GradientLibrary::instance();
  QStringList failed;
  std::size_t imported = 0;

  for (const QString &file : files) {
    const GradientImportReport report =
        lib.import_file(std::filesystem::path(file.toStdString()));
    if (!report.ok)
      failed << QFileInfo(file).fileName();
    else
      imported += report.added + report.renamed;
  }

  Logger::log()->trace("GradientPicker::on_import_clicked: {} imported from "
                       "{} file(s), {} failed",
                       imported, files.size(), failed.size());

  if (!failed.isEmpty())
    QMessageBox::warning(
        this, tr("Import gradients"),
        tr("No gradients could be read from:\n%1").arg(failed.join('\n')));
}

void GradientPicker::on_export_clicked() {
  export_presets(GradientLibrary::instance().presets(), "gradients.json");
}

void GradientPicker::export_presets(const std::vector<Preset> &presets,
                                    const QString &suggested_file) {
  QString file = QFileDialog::getSaveFileName(
      this, tr("Export gradients"), suggested_file, gradient_file_filter());
  if (file.isEmpty())
    return;
  if (!file.endsWith(".json", Qt::CaseInsensitive))
    file += ".json";

  if (!GradientLibrary::instance().export_file(
          std::filesystem::path(file.toStdString()), presets))
    QMessageBox::warning(this, tr("Export gradients"),
                         tr("Could not write \"%1\".").arg(file));
}

void GradientPicker::show_entry_menu(Entry entry, const QPoint &global_pos) {
  // `entry` is a copy on purpose: the menu runs a nested event loop during
  // which entries_ may be rebuilt.
  GradientLibrary &lib = GradientLibrary::instance();
  const std::string name = entry.preset.name;

  QMenu menu(this);

  QAction *favorite =
      menu.addAction(lib.is_favorite(name) ? tr("Remove from favorites")
                                           : tr("Add to favorites"));

  QAction *rename = nullptr;
  QAction *replace = nullptr;
  QAction *remove = nullptr;
  if (entry.user) {
    menu.addSeparator();
    rename = menu.addAction(tr("Rename..."));
    replace = menu.addAction(tr("Replace with current gradient"));
    remove = menu.addAction(tr("Delete"));
  }

  menu.addSeparator();
  QAction *export_action = menu.addAction(tr("Export..."));

  QAction *chosen = menu.exec(global_pos);
  if (!chosen)
    return;

  if (chosen == favorite) {
    lib.set_favorite(name, !lib.is_favorite(name));
  } else if (chosen == rename) {
    bool ok = false;
    const QString text = QInputDialog::getText(
        this, tr("Rename gradient"), tr("New name:"), QLineEdit::Normal,
        QString::fromStdString(name), &ok);
    if (!ok || text.trimmed().isEmpty())
      return;

    if (!lib.rename(name, text.trimmed().toStdString()))
      QMessageBox::warning(
          this, tr("Rename gradient"),
          tr("A gradient named \"%1\" already exists.").arg(text.trimmed()));
  } else if (chosen == replace) {
    lib.update(name, stops_);
  } else if (chosen == remove) {
    const auto answer =
        QMessageBox::question(this, tr("Delete gradient"),
                              tr("Delete \"%1\" from your library?")
                                  .arg(QString::fromStdString(name)));
    if (answer == QMessageBox::Yes)
      lib.remove(name);
  } else if (chosen == export_action) {
    export_presets({entry.preset}, QString::fromStdString(name) + ".json");
  }
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

void GradientPicker::resizeEvent(QResizeEvent *e) {
  QWidget::resizeEvent(e);
  if (scroll_area_ && scroll_area_->viewport() && preset_grid_) {
    preset_grid_->reflow(scroll_area_->viewport()->width());
  }
}

bool GradientPicker::eventFilter(QObject *watched, QEvent *event) {
  if (scroll_area_ && watched == scroll_area_->viewport() &&
      event->type() == QEvent::Resize) {
    auto *re = static_cast<QResizeEvent *>(event);
    if (preset_grid_)
      preset_grid_->reflow(re->size().width());
  }
  return QWidget::eventFilter(watched, event);
}

QSize GradientPicker::sizeHint() const {
  const int top_h = GradientBarWidget::TOTAL_H + 4 + TOOLBAR_H;
  const int preset_h = entries_.empty() ? 0 : (SWATCH_H + 4) * 3 + 8;
  return {300, top_h + (entries_.empty() ? 0 : 4 + preset_h)};
}

QSize GradientPicker::minimumSizeHint() const {
  const int top_h = GradientBarWidget::TOTAL_H + 4 + TOOLBAR_H;
  const int preset_h = entries_.empty() ? 0 : SWATCH_H + 8;
  return {160, top_h + (entries_.empty() ? 0 : 4 + preset_h)};
}

} // namespace meta::qt
