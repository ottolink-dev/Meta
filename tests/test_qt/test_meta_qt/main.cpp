#include <algorithm>
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <random>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "meta.hpp"
#include "meta_qt.hpp"
#include "meta_qt/widgets/points_canvas.hpp"
#include "meta_qt/widgets/range_bar.hpp"

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

std::string truncate(const std::string &s, size_t max_len = 80)
{
  if (s.size() <= max_len) return s;
  return s.substr(0, max_len) + "...";
}

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES

inline std::vector<meta::Preset> generate_random_presets(
    size_t   n,
    uint32_t seed = std::random_device{}())
{
  std::mt19937 rng(seed);

  std::uniform_int_distribution<int>    stop_count_dist(2, 8);
  std::uniform_real_distribution<float> pos_dist(0.f, 1.f);
  std::uniform_real_distribution<float> color_dist(0.f, 1.f);

  std::vector<meta::Preset> presets;
  presets.reserve(n);

  for (size_t i = 0; i < n; ++i)
  {
    meta::Preset preset;
    preset.name = "Preset " + std::to_string(i + 1);

    int stop_count = stop_count_dist(rng);
    preset.stops.reserve(stop_count);

    preset.stops.push_back(
        {0.f, {color_dist(rng), color_dist(rng), color_dist(rng), 1.f}});

    for (int j = 1; j < stop_count - 1; ++j)
    {
      preset.stops.push_back(
          {pos_dist(rng),
           {color_dist(rng), color_dist(rng), color_dist(rng), 1.f}});
    }

    preset.stops.push_back(
        {1.f, {color_dist(rng), color_dist(rng), color_dist(rng), 1.f}});

    std::sort(preset.stops.begin() + 1,
              preset.stops.end() - 1,
              [](const meta::Stop &a, const meta::Stop &b)
              { return a.position < b.position; });

    presets.push_back(std::move(preset));
  }

  return presets;
}

#endif

// -----------------------------------------------------------------------------
// Debug view
// -----------------------------------------------------------------------------

QWidget *make_debug_view(meta::AbstractAttribute *p_attr,
                         bool                     add_border = false)
{
  auto *base = new QWidget();

  auto *widget = meta::qt::render(p_attr);

  auto *label1 = new QLabel(
      QString::fromStdString("Value = " + truncate(p_attr->to_string())));

  auto *label2 = new QLabel(
      QString::fromStdString("Ended = " + truncate(p_attr->to_string())));

  auto *layout = new QVBoxLayout();
  layout->addWidget(widget);
  layout->addWidget(label1);
  layout->addWidget(label2);

  base->setLayout(layout);

  QObject::connect(widget,
                   &meta::qt::MetaWidget::edit_ended,
                   base,
                   [label2, p_attr]()
                   {
                     label2->setText(QString::fromStdString(
                         "Started = " + truncate(p_attr->to_string())));
                   });

  QObject::connect(widget,
                   &meta::qt::MetaWidget::value_changed,
                   base,
                   [label1, p_attr]()
                   {
                     label1->setText(QString::fromStdString(
                         " - Value = " + truncate(p_attr->to_string())));
                   });

  QObject::connect(widget,
                   &meta::qt::MetaWidget::edit_ended,
                   base,
                   [label2, p_attr]()
                   {
                     label2->setText(QString::fromStdString(
                         "   + Ended = " + truncate(p_attr->to_string())));
                   });

  if (add_border)
  {
    base->setStyleSheet("background: red;");
    widget->setStyleSheet("background: green;");
  }

  base->show();

  return base;
}

// -----------------------------------------------------------------------------
// Container rendering
// -----------------------------------------------------------------------------

QWidget *make_container_view(meta::AttributeContainer &container,
                             bool                      snapshots = false)
{
  auto *scroll = new QScrollArea();
  scroll->setWidgetResizable(true);

  auto *content = new QWidget();
  auto *layout = new QVBoxLayout(content);

  layout->setContentsMargins(4, 4, 4, 4);

  meta::qt::ContainerRenderOptions options;
  options.category_policy = meta::qt::CategoryPolicy::CP_MERGED;
  options.snapshot_manager = snapshots;

  auto *widget = meta::qt::render(container, options);

  widget->setMinimumWidth(500);

  QObject::connect(widget,
                   &meta::qt::MetaWidget::edit_started,
                   []() { std::cout << "+ edit_started\n"; });

  QObject::connect(widget,
                   &meta::qt::MetaWidget::value_changed,
                   []() { std::cout << "  - value_changed\n"; });

  QObject::connect(widget,
                   &meta::qt::MetaWidget::edit_ended,
                   []() { std::cout << "    > edit_ended\n"; });

  layout->addWidget(widget);

  content->setLayout(layout);
  scroll->setWidget(content);

  return scroll;
}

QWidget *make_group_view(meta::ContainerGroup &group)
{
  auto *scroll = new QScrollArea();
  scroll->setWidgetResizable(true);

  auto *content = new QWidget();
  auto *layout = new QVBoxLayout(content);

  layout->setContentsMargins(4, 4, 4, 4);

  meta::qt::ContainerRenderOptions options;
  options.category_policy = meta::qt::CategoryPolicy::CP_TREE;
  options.collapse_regex = std::regex("^Cat 1");
  options.snapshot_manager = true;

  auto *widget = meta::qt::render(group, options);

  widget->setMinimumWidth(500);

  QObject::connect(widget,
                   &meta::qt::MetaWidget::edit_started,
                   []() { std::cout << "+ edit_started\n"; });

  QObject::connect(widget,
                   &meta::qt::MetaWidget::value_changed,
                   []() { std::cout << "  - value_changed\n"; });

  QObject::connect(widget,
                   &meta::qt::MetaWidget::edit_ended,
                   []() { std::cout << "    > edit_changed\n"; });

  layout->addWidget(widget);

  content->setLayout(layout);
  scroll->setWidget(content);

  return scroll;
}

// -----------------------------------------------------------------------------
// Bool tests
// -----------------------------------------------------------------------------

void add_bool_tests(meta::AttributeContainer &container)
{
  {
    auto *a = container.add("bool_toggle", true);
    a->metadata().add(meta::keys::ui::label, "Button Label");
    a->metadata().add(meta::keys::ui::widget_type, "Toggle");
  }

  {
    auto *a = container.add("bool_checkbox", true);
    a->metadata().add(meta::keys::ui::label, "Button Label");
    a->metadata().add(meta::keys::ui::widget_type, "Checkbox");
  }

  {
    auto *a = container.add("bool_binary_buttons", true);
    a->metadata().add(meta::keys::ui::widget_type, "BinaryButtons");
    a->metadata().add(meta::keys::ui::label, "Button Label");
    a->metadata().add(meta::keys::ui::label_true, "True V");
    a->metadata().add(meta::keys::ui::label_false, "False V");
  }
}

// -----------------------------------------------------------------------------
// Float tests
// -----------------------------------------------------------------------------

void add_float_tests(meta::AttributeContainer &container)
{
  {
    auto *a = container.add("float_input", 0.f);
    a->metadata().add(meta::keys::ui::widget_type, "Input");
    a->metadata().add(meta::keys::constraints::min, -1.f);
    a->metadata().add(meta::keys::ui::format, "{:.3f}");
  }

  {
    auto *a = container.add("float_slider", 0.f);
    a->metadata().add(meta::keys::ui::widget_type, "Slider");
    a->metadata().add(meta::keys::constraints::min, -1.f);
    a->metadata().add(meta::keys::constraints::max, 3.f);
    a->metadata().add(meta::keys::constraints::step, 0.2f);
    a->metadata().add(meta::keys::ui::format, "{:.2f}");
  }

  {
    auto *a = container.add("float_scrollbar", 0.f);
    a->metadata().add(meta::keys::ui::widget_type, "ScrollBar");
    a->metadata().add(meta::keys::constraints::min, -1.f);
    a->metadata().add(meta::keys::constraints::max, 3.f);
    a->metadata().add(meta::keys::constraints::step, 0.2f);
  }

  {
    auto *a = container.add("float_dial", 0.f);
    a->metadata().add(meta::keys::ui::widget_type, "Dial");
    a->metadata().add(meta::keys::constraints::min, -1.f);
    a->metadata().add(meta::keys::constraints::max, 3.f);
  }

  {
    auto *a = container.add("float_slider_custom", 0.f);
    a->metadata().add(meta::keys::ui::widget_type, "SliderFloat");
    a->metadata().add(meta::keys::constraints::min, -1.f);
    a->metadata().add(meta::keys::constraints::max, 3.f);
    a->metadata().add(meta::keys::constraints::step, 0.2f);
    a->metadata().add(meta::keys::ui::format, "{:.2f}");
    a->metadata().add("ui.plus_minus", true);
  }

  {
    auto *a = container.add("float_slider_custom_unbounded", 0.f);
    a->metadata().add(meta::keys::ui::widget_type, "SliderFloat");
    a->metadata().add(meta::keys::constraints::min, -1.f);
    a->metadata().add(meta::keys::constraints::max, FLT_MAX);
    a->metadata().add(meta::keys::constraints::step, 0.2f);
    a->metadata().add(meta::keys::ui::format, "{:.2f}");
    a->metadata().add("ui.plus_minus", true);
  }

  {
    auto *a = container.add("float_slider_log", 0.f);
    a->metadata().add(meta::keys::ui::widget_type, "SliderFloat");
    a->metadata().add(meta::keys::constraints::min, 0.001f);
    a->metadata().add(meta::keys::constraints::max, 100.f);
    a->metadata().add(meta::keys::ui::format, "{:.3e}");
    a->metadata().add("ui.log_scale", true);
  }
}

// -----------------------------------------------------------------------------
// Int tests
// -----------------------------------------------------------------------------

void add_int_tests(meta::AttributeContainer &container)
{
  {
    auto *a = container.add("int_enumcombobox", 0);

    a->metadata().add(meta::keys::ui::widget_type, "EnumComboBox");

    std::vector<std::pair<int, std::string>> options = {{0, "Linear"},
                                                        {1, "Cubic"},
                                                        {2, "Bezier"}};

    a->metadata().add(meta::keys::constraints::enum_items, options);
  }

  {
    auto *a = container.add("int_input", 0);
    a->metadata().add(meta::keys::ui::widget_type, "Input");
  }

  {
    auto *a = container.add("int_slider", 0);
    a->metadata().add(meta::keys::ui::widget_type, "Slider");
    a->metadata().add(meta::keys::constraints::min, -1);
    a->metadata().add(meta::keys::constraints::max, 3);
  }

  {
    auto *a = container.add("int_scrollbar", 0);
    a->metadata().add(meta::keys::ui::widget_type, "ScrollBar");
    a->metadata().add(meta::keys::constraints::min, -1);
    a->metadata().add(meta::keys::constraints::max, 3);
  }

  {
    auto *a = container.add("int_dial", 0);
    a->metadata().add(meta::keys::ui::widget_type, "Dial");
    a->metadata().add(meta::keys::constraints::min, -1);
    a->metadata().add(meta::keys::constraints::max, 3);
  }

  {
    auto *a = container.add("int_slider_custome", 0);
    a->metadata().add(meta::keys::ui::widget_type, "SliderInt");
    a->metadata().add(meta::keys::constraints::min, -1);
    a->metadata().add(meta::keys::constraints::max, 10);
    a->metadata().add("ui.plus_minus", true);
  }
}

// -----------------------------------------------------------------------------
// String tests
// -----------------------------------------------------------------------------

void add_string_tests(meta::AttributeContainer &container)
{
  auto options = std::vector<std::string>{"Option A",
                                          "Option B",
                                          "Option C",
                                          "Option D"};

  {
    auto *a = container.add("string_combobox", "Option B");
    a->metadata().add(meta::keys::ui::widget_type, "ComboBox");
    a->metadata().add(meta::keys::constraints::allowed_values, options);
  }

  {
    auto *a = container.add("string_buttongrid", "Option B");
    a->metadata().add(meta::keys::ui::widget_type, "ButtonGrid");
    a->metadata().add(meta::keys::constraints::allowed_values, options);
  }

  {
    auto *a = container.add("string_single_line", "");
    a->metadata().add(meta::keys::ui::widget_type, "SingleLineText");
    a->metadata().add("placeholder", "Text goes here...");
  }

  {
    auto *a = container.add("string_multiline", "");
    a->metadata().add(meta::keys::ui::widget_type, "MultilineText");
    a->metadata().add("ui.placeholder", "Text goes here...");
    a->metadata().add("ui.min_lines", 12);
  }

  {
    auto *a = container.add("string_editor", "");
    a->metadata().add(meta::keys::ui::widget_type, "CodeEditor");
    a->metadata().add("ui.min_lines", 24);
  }

  {
    auto *a = container.add("string_readonly", "Read-only text content");
    a->metadata().add(meta::keys::ui::widget_type, "ReadOnlyText");
  }
}

// -----------------------------------------------------------------------------
// std tests
// -----------------------------------------------------------------------------

void add_std_tests(meta::AttributeContainer &container)
{
  {
    auto *a = container.add("std::filesystem::path_open",
                            std::filesystem::path());

    a->metadata().add(meta::keys::ui::widget_type, "OpenFile");
    a->metadata().add("ui.start_dir", ".");
  }

  {
    auto *a = container.add("std::filesystem::path_save",
                            std::filesystem::path("./some_file"));

    a->metadata().add(meta::keys::ui::widget_type, "SaveFile");
  }

  {
    auto *a = container.add("std::filesystem::path_dir",
                            std::filesystem::path());

    a->metadata().add(meta::keys::ui::widget_type, "Directory");
  }

  {
    std::vector<float> values = {0.f, 0.1f, 0.75f, 0.5f};

    container.add("std::vector::float", values);
  }
}

// -----------------------------------------------------------------------------
// GLM tests
// -----------------------------------------------------------------------------

#ifdef META_ENABLE_GLM_TYPES

void add_glm_tests(meta::AttributeContainer &container)
{
  // ---------------------------------------------------------------------------
  // glm::ivec2
  // ---------------------------------------------------------------------------

  {
    container.add("glm::ivec2_free", glm::ivec2(16, 32));
  }

  {
    auto *a = container.add("glm::ivec2_constrained", glm::ivec2(16, 32));

    a->metadata().add(meta::keys::constraints::min, 16);
    a->metadata().add(meta::keys::constraints::max, int(std::pow(2, 16)));
    a->metadata().add(meta::keys::constraints::power_of_two, true);
    a->metadata().add(meta::keys::constraints::aspect_ratio, 4.f);
  }

  // ---------------------------------------------------------------------------
  // glm::vec2
  // ---------------------------------------------------------------------------

  {
    container.add("glm::vec2_free", glm::vec2(16, 32));
  }

  {
    auto *a = container.add("glm::vec2_constrained", glm::vec2(16.f, 32.f));

    a->metadata().add(meta::keys::constraints::min, -1.f);
    a->metadata().add(meta::keys::constraints::max, 64.f);
    a->metadata().add(meta::keys::constraints::step, 0.1f);
    a->metadata().add(meta::keys::ui::format, "{:.2f}");
  }

  {
    auto *a = container.add("glm::vec2_xy", glm::vec2(16.f, 32.f));

    a->metadata().add(meta::keys::ui::widget_type, "XYCanvas");
    a->metadata().add(meta::keys::constraints::min, -1.f);
    a->metadata().add(meta::keys::constraints::max, 64.f);
  }

  {
    auto *a = container.add("glm::vec2_vector", glm::vec2(16.f, 16.f));

    a->metadata().add(meta::keys::ui::widget_type, "VectorEditor");
    a->metadata().add(meta::keys::constraints::min, 0.f);
    a->metadata().add(meta::keys::constraints::max, 128.f);
    a->state().add(meta::keys::state::locked_xy, true);
  }

  {
    auto *a = container.add("glm::vec2_linked", glm::vec2(16.f, 32.f));

    a->metadata().add(meta::keys::ui::widget_type, "LinkedSliders");
    a->metadata().add(meta::keys::constraints::min, 0.f);
    a->metadata().add(meta::keys::constraints::max, 64.f);
    a->state().add(meta::keys::state::locked_xy, true);
    a->metadata().add("ui.label_x", "kx");
    a->metadata().add("ui.label_y", "ky");
    a->metadata().add(meta::keys::ui::format, "{:.1f}");
  }

  {
    auto *a = container.add("glm::vec2_linked_opened", glm::vec2(16.f, 32.f));

    a->metadata().add(meta::keys::ui::widget_type, "LinkedSliders");
    a->metadata().add(meta::keys::constraints::min, 0.f);
    a->metadata().add(meta::keys::constraints::max, FLT_MAX);
    a->state().add(meta::keys::state::locked_xy, true);
    a->metadata().add("ui.label_x", "kx");
    a->metadata().add("ui.label_y", "ky");
    a->metadata().add(meta::keys::ui::format, "{:.1f}");
  }

  {
    auto *a = container.add("glm::vec2_range", glm::vec2(0.f, 1.f));

    a->metadata().add(meta::keys::ui::widget_type, "RangeBar");
    a->metadata().add(meta::keys::constraints::min, -1.f);
    a->metadata().add(meta::keys::constraints::max, 2.f);
    a->metadata().add(meta::keys::constraints::step, 0.1f);
    a->metadata().add(meta::keys::ui::format, "{:.3f}");

    a->metadata().add(meta::keys::ui::data_provider,
                      meta::DataProvider(
                          []()
                          {
                            meta::qt::HistogramData hist;

                            hist.x = {-0.5f, 0.0f, 0.5f, 1.0f, 1.5f};

                            hist.y = {0.1f, 0.8f, 0.4f, 0.9f, 0.2f};

                            return hist;
                          }));
  }

  // ---------------------------------------------------------------------------
  // glm::vec3
  // ---------------------------------------------------------------------------

  {
    container.add("glm::vec3", glm::vec3(16.f, 32.f, 64.f));
  }

  {
    auto *a = container.add("glm::vec3_constrained",
                            glm::vec3(16.f, 32.f, 64.f));

    a->metadata().add(meta::keys::constraints::min, -1.f);
    a->metadata().add(meta::keys::constraints::max, 64.f);
    a->metadata().add(meta::keys::constraints::step, 0.1f);
    a->metadata().add(meta::keys::ui::format, "{:.1f}");
  }

  {
    auto *a = container.add("glm::vec3_color", glm::vec3(0.5f, 0.1f, 0.f));

    a->metadata().add(meta::keys::ui::widget_type, "ColorPicker");
  }

  // ---------------------------------------------------------------------------
  // glm::vec4
  // ---------------------------------------------------------------------------

  {
    container.add("glm::vec4", glm::vec4(16.f, 32.f, 64.f, 128.f));
  }

  {
    auto *a = container.add("glm::vec4_color",
                            glm::vec4(0.5f, 0.1f, 0.f, 0.5f));

    a->metadata().add(meta::keys::ui::widget_type, "ColorPicker");
  }

  // ---------------------------------------------------------------------------
  // PointsEditor
  // ---------------------------------------------------------------------------

  {
    std::vector<glm::vec3> values = {glm::vec3(0.1f, 0.2f, 0.1f),
                                     glm::vec3(0.5f, 0.25f, 0.5f),
                                     glm::vec3(0.7f, 0.5f, 1.f)};

    auto *a = container.add("std::vector<glm::vec3>_points", values);

    a->metadata().add(meta::keys::ui::widget_type, "PointsEditor");

    a->metadata().add(
        meta::keys::ui::data_provider,
        meta::DataProvider(
            []()
            {
              meta::qt::ImageData img;

              img.width = 4;
              img.height = 4;
              img.channels = 3;

              img.pixels = std::vector<uint8_t>(4 * 4 * 3, 200);

              for (int y = 0; y < 4; ++y)
              {
                for (int x = 0; x < 4; ++x)
                {
                  int idx = (y * 4 + x) * 3;

                  img.pixels[idx] = static_cast<uint8_t>((x + y) * 32);

                  img.pixels[idx + 1] = static_cast<uint8_t>(x * 64);

                  img.pixels[idx + 2] = static_cast<uint8_t>(y * 64);
                }
              }

              return img;
            }));
  }

  // ---------------------------------------------------------------------------
  // PathEditor
  // ---------------------------------------------------------------------------

  {
    std::vector<glm::vec3> values = {glm::vec3(0.1f, 0.2f, 0.1f),
                                     glm::vec3(0.5f, 0.25f, 0.5f),
                                     glm::vec3(0.7f, 0.5f, 1.f)};

    auto *a = container.add("std::vector<glm::vec3>_path", values);

    a->metadata().add(meta::keys::ui::widget_type, "PathEditor");
  }
}

#endif

// -----------------------------------------------------------------------------
// Color gradient tests
// -----------------------------------------------------------------------------

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES

void add_color_gradient_tests(meta::AttributeContainer &container)
{
  auto *a = container.add("ColorGradient", meta::ColorGradient());

  a->metadata().add(meta::keys::ui::presets,
                    meta::GradientPresets{generate_random_presets(512)});
}

#endif

// -----------------------------------------------------------------------------
// Array tests
// -----------------------------------------------------------------------------

#ifdef META_ENABLE_ARRAY_TYPES

void add_array_tests(meta::AttributeContainer &container)
{
  meta::Array arr;

  arr.shape = {128, 128};
  arr.vector.resize(arr.shape.x * arr.shape.y, 0.f);

  float xc = 0.5f * arr.shape.x;
  float yc = 0.5f * arr.shape.y;

  for (int y = 0; y < arr.shape.y; ++y)
  {
    for (int x = 0; x < arr.shape.x; ++x)
    {
      float dx = (x - xc) / xc;
      float dy = (y - yc) / yc;

      float dist = std::sqrt(dx * dx + dy * dy);

      arr.vector[static_cast<size_t>(y * arr.shape.x + x)] = std::clamp(
          1.f - dist,
          0.f,
          1.f);
    }
  }

  auto *a = container.add("Array", arr);

  // The editor shape (not the array shape).
  a->metadata().add(meta::keys::ui::width, 512);

  a->metadata().add(meta::keys::ui::height, 512);

  a->metadata().add(
      meta::keys::ui::data_provider,
      meta::DataProvider(
          []()
          {
            meta::qt::ImageData img;

            img.width = 312;
            img.height = 312;
            img.channels = 4;

            img.pixels.resize(
                static_cast<size_t>(img.width * img.height * img.channels),
                0);

            for (int y = 0; y < img.height; ++y)
            {
              for (int x = 0; x < img.width; ++x)
              {
                float u = static_cast<float>(x) /
                          static_cast<float>(img.width - 1);

                float v = static_cast<float>(y) /
                          static_cast<float>(img.height - 1);

                float r = (1.f - u) * (1.f - v) * 255.f + u * v * 255.f;

                float g = u * (1.f - v) * 255.f;

                float b = (1.f - u) * v * 255.f + u * v * 255.f;

                float alpha = (1.f - u) * (1.f - v) * 255.f +
                              u * (1.f - v) * 255.f + (1.f - u) * v * 255.f;

                size_t idx = static_cast<size_t>((y * img.width + x) *
                                                 img.channels);

                img.pixels[idx] = static_cast<uint8_t>(r);

                img.pixels[idx + 1] = static_cast<uint8_t>(g);

                img.pixels[idx + 2] = static_cast<uint8_t>(b);

                img.pixels[idx + 3] = static_cast<uint8_t>(alpha);
              }
            }

            return img;
          }));
}

#endif

// -----------------------------------------------------------------------------
// ContainerGroup tests
// -----------------------------------------------------------------------------

void add_group_tests(meta::ContainerGroup &group)
{
  auto &node_settings = group.add("node_settings");
  auto &ui_settings = group.add("ui_settings");
  auto &debug_settings = group.add("debug_settings");

  group.set_current("node_settings");

  // ---------------------------------------------------------------------------
  // Node settings
  // ---------------------------------------------------------------------------

  {
    auto *a = node_settings.add("threshold", 0.5f);

    a->metadata().add(meta::keys::constraints::min, 0.f);
    a->metadata().add(meta::keys::constraints::max, 5.f);
    a->metadata().add(meta::keys::ui::widget_type, "Slider");
    a->metadata().add(meta::keys::ui::category, "Base/Something/Category 2");
  }

  {
    auto *a = node_settings.add("iterations", 8);

    a->metadata().add(meta::keys::ui::category, "Base/Cat 1");
  }

  node_settings.add("active", true);

  // ---------------------------------------------------------------------------
  // UI settings
  // ---------------------------------------------------------------------------

  ui_settings.add("theme", std::string("dark"));
  ui_settings.add("font_size", 14.f);
  ui_settings.add("show_grid", true);

  // ---------------------------------------------------------------------------
  // Debug settings
  // ---------------------------------------------------------------------------

  debug_settings.add("log_level", 2);
  debug_settings.add("wireframe", false);
  debug_settings.add("draw_bounds", true);
  debug_settings.add("show_grid", true);

  // ---------------------------------------------------------------------------
  // Presets
  // ---------------------------------------------------------------------------

  meta::presets::seed(node_settings, "seed", "Random Seed");
  meta::presets::angle(node_settings, "angle", "Angle");

  group.synchronize_all();
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  // ---------------------------------------------------------------------------
  // Enable/disable test categories
  // ---------------------------------------------------------------------------

  const bool base_bool = false;
  const bool base_float = true;
  const bool base_int = false;

  const bool base_string = false;
  const bool base_std = false;

#ifdef META_ENABLE_GLM_TYPES
  const bool base_glm = true;
#else
  const bool base_glm = false;
#endif

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
  const bool base_color_gradient = true;
#else
  const bool base_color_gradient = false;
#endif

#ifdef META_ENABLE_ARRAY_TYPES
  const bool base_array = true;
#else
  const bool base_array = false;
#endif

  const bool base_groups = true;

  // ---------------------------------------------------------------------------
  // Containers
  // ---------------------------------------------------------------------------

  meta::AttributeContainer bool_container;
  meta::AttributeContainer float_container;
  meta::AttributeContainer int_container;
  meta::AttributeContainer string_container;
  meta::AttributeContainer std_container;
  meta::AttributeContainer glm_container;
  meta::AttributeContainer color_gradient_container;
  meta::AttributeContainer array_container;

  // ---------------------------------------------------------------------------
  // Populate containers
  // ---------------------------------------------------------------------------

  if (base_bool)
  {
    add_bool_tests(bool_container);
  }

  if (base_float)
  {
    add_float_tests(float_container);
  }

  if (base_int)
  {
    add_int_tests(int_container);
  }

  if (base_string)
  {
    add_string_tests(string_container);
  }

  if (base_std)
  {
    add_std_tests(std_container);
  }

#ifdef META_ENABLE_GLM_TYPES
  if (base_glm)
  {
    add_glm_tests(glm_container);
  }
#endif

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
  if (base_color_gradient)
  {
    add_color_gradient_tests(color_gradient_container);
  }
#endif

#ifdef META_ENABLE_ARRAY_TYPES
  if (base_array)
  {
    add_array_tests(array_container);
  }
#endif

  // ---------------------------------------------------------------------------
  // ContainerGroup
  // ---------------------------------------------------------------------------

  meta::ContainerGroup group;

  if (base_groups)
  {
    add_group_tests(group);
  }

  // ---------------------------------------------------------------------------
  // Qt
  // ---------------------------------------------------------------------------

  QApplication app(argc, argv);

  auto *tabs = new QTabWidget();

  tabs->setDocumentMode(true);

  // ---------------------------------------------------------------------------
  // Individual attribute containers
  // ---------------------------------------------------------------------------

  if (base_bool)
  {
    tabs->addTab(make_container_view(bool_container), "Bool");
  }

  if (base_float)
  {
    tabs->addTab(make_container_view(float_container), "Float");
  }

  if (base_int)
  {
    tabs->addTab(make_container_view(int_container), "Int");
  }

  if (base_string)
  {
    tabs->addTab(make_container_view(string_container), "String");
  }

  if (base_std)
  {
    tabs->addTab(make_container_view(std_container), "std");
  }

#ifdef META_ENABLE_GLM_TYPES
  if (base_glm)
  {
    tabs->addTab(make_container_view(glm_container), "GLM");
  }
#endif

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
  if (base_color_gradient)
  {
    tabs->addTab(make_container_view(color_gradient_container),
                 "Color Gradient");
  }
#endif

#ifdef META_ENABLE_ARRAY_TYPES
  if (base_array)
  {
    tabs->addTab(make_container_view(array_container), "Array");
  }
#endif

  // ---------------------------------------------------------------------------
  // ContainerGroup
  // ---------------------------------------------------------------------------

  if (base_groups)
  {
    tabs->addTab(make_group_view(group), "ContainerGroup");
  }

  // ---------------------------------------------------------------------------
  // Optional debug view
  // ---------------------------------------------------------------------------

  if (false)
  {
    bool add_border = false;

    for (const auto &[name, sp_attr] : float_container)
    {
      (void)name;
      make_debug_view(sp_attr.get(), add_border);
    }
  }

  // ---------------------------------------------------------------------------
  // Optional serialization debug
  // ---------------------------------------------------------------------------

  if (false)
  {
    auto attr = meta::Attribute("debug",
                                std::string(float_container.json_to().dump(4)));

    attr.metadata().add(meta::keys::ui::widget_type, "CodeEditor");
    attr.metadata().add("ui.min_lines", 24);

    make_debug_view(&attr);
  }

  // ---------------------------------------------------------------------------
  // Snapshot test
  // ---------------------------------------------------------------------------

  if (false)
  {
    float_container.snapshot_manager().save("default",
                                            float_container.json_to());

    float_container.snapshot_manager().save("Some Config.",
                                            float_container.json_to());

    tabs->addTab(make_container_view(float_container, true),
                 "Float + Snapshots");
  }

  // ---------------------------------------------------------------------------
  // Multiple render test
  // ---------------------------------------------------------------------------

  if (true)
  {
    auto *widget1 = meta::qt::render(float_container);
    auto *widget2 = meta::qt::render(float_container);

    widget1->show();
    widget2->show();
  }

  // ---------------------------------------------------------------------------
  // NEW WIDGET test
  // ---------------------------------------------------------------------------

  if (true)
  {
    auto *button = new QPushButton("NEW WIDGET");

    button->setCheckable(true);

    QObject::connect(
        button,
        &QPushButton::toggled,
        [&group]()
        {
          meta::qt::ContainerRenderOptions options;
          options.category_policy = meta::qt::CategoryPolicy::CP_TREE;
          options.collapse_regex = std::regex("^Cat 1");

          auto *widget = meta::qt::render(group, options);

          widget->show();
        });

    button->show();
  }

  // ---------------------------------------------------------------------------
  // Main window
  // ---------------------------------------------------------------------------

  tabs->resize(900, 800);
  tabs->show();

  return app.exec();
}
