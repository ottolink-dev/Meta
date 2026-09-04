/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include <algorithm>
#include <cmath>

#include <QColorDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

#include "meta/logger.hpp"
#include "meta_qt/widgets/gradient_picker.hpp"

namespace meta::qt
{

// ---------------------------------------------------------------------------
// Colour conversion helpers
// ---------------------------------------------------------------------------

static QColor to_qcolor(const std::array<float, 4> &c)
{
  return QColor(int(std::clamp(c[0], 0.f, 1.f) * 255.f),
                int(std::clamp(c[1], 0.f, 1.f) * 255.f),
                int(std::clamp(c[2], 0.f, 1.f) * 255.f),
                int(std::clamp(c[3], 0.f, 1.f) * 255.f));
}

static std::array<float, 4> from_qcolor(const QColor &c)
{
  return {float(c.redF()),
          float(c.greenF()),
          float(c.blueF()),
          float(c.alphaF())};
}

// ---------------------------------------------------------------------------
// GradientBarWidget
// ---------------------------------------------------------------------------

GradientBarWidget::GradientBarWidget(std::vector<Stop> &stops, QWidget *parent)
    : QWidget(parent), stops_(stops)
{
  setFixedHeight(TOTAL_H);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void GradientBarWidget::sort_stops()
{
  std::sort(stops_.begin(),
            stops_.end(),
            [](const Stop &a, const Stop &b)
            { return a.position < b.position; });
}

void GradientBarWidget::paintEvent(QPaintEvent *)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  const QRectF    br = bar_rect();
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
  for (int i = 0; i < static_cast<int>(stops_.size()); ++i)
  {
    const QRectF r = stop_rect(stops_[i]);
    const bool   sel = (i == selected_idx_);

    // Small triangle pointing up from bar bottom to handle
    const float cx = float(r.center().x());
    const float ty = float(br.bottom());
    QPolygonF   tri;
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

void GradientBarWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
  const int idx = hit_test(e->pos());

  if (idx >= 0)
  {
    // Edit existing stop colour
    const QColor picked = QColorDialog::getColor(
        to_qcolor(stops_[idx].color),
        this,
        QString(),
        QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);

    if (picked.isValid())
    {
      stops_[idx].color = from_qcolor(picked);
      update();
      Q_EMIT value_changed();
      Q_EMIT edit_ended();
    }
  }
  else if (bar_rect().contains(QPointF(e->pos())))
  {
    // Add new stop
    const double pos = std::clamp((e->pos().x() - bar_rect().left()) /
                                      bar_rect().width(),
                                  0.0,
                                  1.0);

    constexpr double eps = 1e-3;
    const bool       too_close = std::any_of(
        stops_.begin(),
        stops_.end(),
        [&](const Stop &s)
        { return std::abs(double(s.position) - pos) < eps; });

    if (!too_close)
    {
      stops_.push_back({float(pos), {1.f, 1.f, 1.f, 1.f}});
      sort_stops();
      auto it = std::find_if(stops_.begin(),
                             stops_.end(),
                             [pos](const Stop &s)
                             { return s.position == float(pos); });
      if (it != stops_.end())
        selected_idx_ = static_cast<int>(std::distance(stops_.begin(), it));
      update();
      Q_EMIT value_changed();
      Q_EMIT edit_ended();
    }
  }
}

void GradientBarWidget::mousePressEvent(QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton)
  {
    selected_idx_ = hit_test(e->pos());
    dragging_ = selected_idx_ >= 0;
    update();
  }
}

void GradientBarWidget::mouseMoveEvent(QMouseEvent *e)
{
  if (!dragging_ || selected_idx_ < 0 ||
      selected_idx_ >= static_cast<int>(stops_.size()))
    return;

  const QRectF br = bar_rect();
  if (br.width() <= 0) return;

  const double pos = std::clamp((e->pos().x() - br.left()) / br.width(),
                                0.0,
                                1.0);

  stops_[selected_idx_].position = float(pos);

  // Maintain sorted order while keeping selected_idx_ tracking the moved stop
  while (selected_idx_ > 0 &&
         stops_[selected_idx_].position < stops_[selected_idx_ - 1].position)
  {
    std::swap(stops_[selected_idx_], stops_[selected_idx_ - 1]);
    --selected_idx_;
  }
  while (selected_idx_ + 1 < static_cast<int>(stops_.size()) &&
         stops_[selected_idx_].position > stops_[selected_idx_ + 1].position)
  {
    std::swap(stops_[selected_idx_], stops_[selected_idx_ + 1]);
    ++selected_idx_;
  }

  update();
  Q_EMIT value_changed();
}

void GradientBarWidget::mouseReleaseEvent(QMouseEvent *)
{
  if (dragging_)
  {
    dragging_ = false;
    Q_EMIT edit_ended();
  }
}

void GradientBarWidget::contextMenuEvent(QContextMenuEvent *e)
{
  const int idx = hit_test(e->pos());
  if (idx < 0) return;

  // Only offer removal when at least 3 stops (keep minimum 2).
  if (static_cast<int>(stops_.size()) <= 2) return;

  QMenu    menu(this);
  QAction *rm = menu.addAction(QObject::tr("Remove stop"));

  if (menu.exec(e->globalPos()) == rm)
  {
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

QRectF GradientBarWidget::bar_rect() const
{
  return QRectF(PAD, PAD, width() - 2 * PAD, BAR_H);
}

QRectF GradientBarWidget::stop_rect(const Stop &s) const
{
  const QRectF br = bar_rect();
  const double cx = br.left() + double(s.position) * br.width();
  const double cy = br.bottom() + 3 + STOP_R;
  return QRectF(cx - STOP_R, cy - STOP_R, STOP_R * 2, STOP_R * 2);
}

int GradientBarWidget::hit_test(const QPoint &pos) const
{
  for (int i = static_cast<int>(stops_.size()) - 1; i >= 0; --i)
    if (stop_rect(stops_[i]).adjusted(-2, -2, 2, 2).contains(QPointF(pos)))
      return i;
  return -1;
}

// ---------------------------------------------------------------------------
// PresetGridWidget: responsive grid that wraps swatches based on width
// ---------------------------------------------------------------------------

class PresetGridWidget : public QWidget
{
public:
  explicit PresetGridWidget(int      swatch_w,
                            int      swatch_h,
                            int      spacing = 4,
                            QWidget *parent = nullptr)
      : QWidget(parent),
        swatch_w_(swatch_w),
        swatch_h_(swatch_h),
        spacing_(spacing)
  {
    // Tell the layout system that our height depends on our width, so
    // QVBoxLayout / QScrollArea can query heightForWidth() instead of
    // relying on a stale, width-independent sizeHint().
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    setSizePolicy(sp);

    init_layout();
  }

  void set_buttons(const std::vector<QPushButton *> &buttons)
  {
    buttons_ = buttons;
    current_cols_ = -1;
    reflow(width());
  }

  void reflow(int avail_w)
  {
    if (buttons_.empty()) return;

    const int cols = compute_cols(avail_w);
    if (cols == current_cols_)
    {
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
      grid_->addWidget(buttons_[i],
                       static_cast<int>(i / cols),
                       static_cast<int>(i % cols));

    const int h = heightForWidth(avail_w);
    setMinimumHeight(h);
    updateGeometry();
  }

  bool hasHeightForWidth() const override { return true; }

  int heightForWidth(int w) const override
  {
    if (buttons_.empty()) return 0;
    const int cols = compute_cols(w);
    const int rows = (static_cast<int>(buttons_.size()) + cols - 1) / cols;
    return 4 + rows * swatch_h_ + (rows - 1) * spacing_;
  }

  QSize sizeHint() const override
  {
    if (buttons_.empty()) return QSize(0, 0);
    const int cols = current_cols_ > 0 ? current_cols_ : 1;
    const int w = 4 + cols * swatch_w_ + (cols - 1) * spacing_;
    return QSize(w, heightForWidth(width() > 0 ? width() : w));
  }

  QSize minimumSizeHint() const override
  {
    if (buttons_.empty()) return QSize(0, 0);
    return QSize(swatch_w_ + 4,
                 heightForWidth(width() > 0 ? width() : swatch_w_ + 4));
  }

protected:
  void resizeEvent(QResizeEvent *event) override
  {
    QWidget::resizeEvent(event);
    reflow(event->size().width());
  }

private:
  int compute_cols(int avail_w) const
  {
    const int margins_w = 4;
    return std::max(1,
                    (avail_w - margins_w + spacing_) / (swatch_w_ + spacing_));
  }

  void init_layout()
  {
    grid_ = new QGridLayout(this);
    grid_->setContentsMargins(2, 2, 2, 2);
    grid_->setSpacing(spacing_);
    grid_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    grid_->setSizeConstraint(QLayout::SetNoConstraint);
  }

  int                        swatch_w_;
  int                        swatch_h_;
  int                        spacing_;
  int                        current_cols_ = -1;
  QGridLayout               *grid_ = nullptr;
  std::vector<QPushButton *> buttons_;
};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

GradientPicker::GradientPicker(std::vector<Stop>         &stops,
                               const std::vector<Preset> &presets,
                               QWidget                   *parent)
    : QWidget(parent), stops_(stops), presets_(presets)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  auto *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(0, 0, 0, 0);
  main_layout->setSpacing(4);

  // Gradient bar pinned at the top (fixed height, never scrolls)
  bar_widget_ = new GradientBarWidget(stops_, this);
  main_layout->addWidget(bar_widget_);

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

  connect(bar_widget_,
          &GradientBarWidget::value_changed,
          this,
          &GradientPicker::value_changed);
  connect(bar_widget_,
          &GradientBarWidget::edit_ended,
          this,
          &GradientPicker::edit_ended);

  rebuild_preset_grid();
}

// ---------------------------------------------------------------------------
// Preset grid
// ---------------------------------------------------------------------------

void GradientPicker::set_presets(const std::vector<Preset> &presets)
{
  presets_ = presets;
  rebuild_preset_grid();
}

void GradientPicker::update_bar()
{
  if (bar_widget_) bar_widget_->update();
}

void GradientPicker::rebuild_preset_grid()
{
  scroll_area_->setVisible(!presets_.empty());

  // Delete all existing child buttons inside preset_grid_
  qDeleteAll(
      preset_grid_->findChildren<QPushButton *>(QString(),
                                                Qt::FindDirectChildrenOnly));

  std::vector<QPushButton *> buttons;
  buttons.reserve(presets_.size());

  for (const auto &preset : presets_)
  {
    QPixmap pix(SWATCH_W, SWATCH_H);
    {
      QPainter pp(&pix);
      pp.setRenderHint(QPainter::Antialiasing);

      QLinearGradient grad(0, 0, pix.width(), 0);
      for (const auto &s : preset.stops)
        grad.setColorAt(double(s.position), to_qcolor(s.color));
      pp.fillRect(pix.rect(), grad);

      // Name overlay
      pp.setPen(Qt::white);
      pp.setFont(QFont(pp.font().family(), 7));
      pp.drawText(pix.rect().adjusted(2, 0, -2, 0),
                  Qt::AlignBottom | Qt::AlignHCenter,
                  QString::fromStdString(preset.name));

      // Border
      pp.setPen(QPen(QColor(80, 80, 80), 1));
      pp.setBrush(Qt::NoBrush);
      pp.drawRoundedRect(pix.rect().adjusted(0, 0, -1, -1),
                         GradientBarWidget::RADIUS,
                         GradientBarWidget::RADIUS);
    }

    auto *btn = new QPushButton(preset_grid_);
    btn->setFixedSize(SWATCH_W, SWATCH_H);
    btn->setFlat(true);
    btn->setIcon(QIcon(pix));
    btn->setIconSize(QSize(SWATCH_W, SWATCH_H));
    btn->setToolTip(QString::fromStdString(preset.name));
    btn->setCursor(Qt::PointingHandCursor);

    const std::vector<Stop> preset_stops = preset.stops;
    connect(btn,
            &QPushButton::clicked,
            this,
            [this, preset_stops]()
            {
              stops_ = preset_stops;
              if (bar_widget_)
              {
                bar_widget_->sort_stops();
                bar_widget_->update();
              }
              Q_EMIT value_changed();
              Q_EMIT edit_ended();
            });

    buttons.push_back(btn);
  }

  preset_grid_->set_buttons(buttons);
  if (scroll_area_ && scroll_area_->viewport())
    preset_grid_->reflow(scroll_area_->viewport()->width());
}

void GradientPicker::resizeEvent(QResizeEvent *e)
{
  QWidget::resizeEvent(e);
  if (scroll_area_ && scroll_area_->viewport() && preset_grid_)
  {
    preset_grid_->reflow(scroll_area_->viewport()->width());
  }
}

bool GradientPicker::eventFilter(QObject *watched, QEvent *event)
{
  if (scroll_area_ && watched == scroll_area_->viewport() &&
      event->type() == QEvent::Resize)
  {
    auto *re = static_cast<QResizeEvent *>(event);
    if (preset_grid_) preset_grid_->reflow(re->size().width());
  }
  return QWidget::eventFilter(watched, event);
}

QSize GradientPicker::sizeHint() const
{
  const int top_h = GradientBarWidget::TOTAL_H;
  const int preset_h = presets_.empty() ? 0 : (SWATCH_H + 4) * 3 + 8;
  return {300, top_h + (presets_.empty() ? 0 : 4 + preset_h)};
}

QSize GradientPicker::minimumSizeHint() const
{
  const int top_h = GradientBarWidget::TOTAL_H;
  const int preset_h = presets_.empty() ? 0 : SWATCH_H + 8;
  return {120, top_h + (presets_.empty() ? 0 : 4 + preset_h)};
}

} // namespace meta::qt
