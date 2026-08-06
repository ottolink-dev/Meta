#include "meta.hpp"
#include "meta_qt.hpp"
#include <QApplication>
#include <iostream>

int main(int argc, char *argv[])
{
  meta::ContainerGroup group;

  auto &numeric_c = group.add("Numeric");
  auto &choice_c = group.add("Choice");
  auto &other_c = group.add("Other");

  // 1. Numeric presets
  meta::presets::slider_float(numeric_c,
                              "slider_float",
                              "Slider Float",
                              0.5f,
                              0.0f,
                              1.0f);
  meta::presets::slider_int(numeric_c, "slider_int", "Slider Int", 5, 0, 10);
  meta::presets::angle(numeric_c, "angle", "Angle", 90.0f);
  meta::presets::seed(numeric_c, "seed", "Random Seed", 12345);

  // 2. Choice presets
  meta::presets::checkbox(choice_c, "checkbox", "Checkbox", true);
  meta::presets::toggle_button(choice_c,
                               "toggle_button",
                               "Toggle Button",
                               true);
  meta::presets::binary_buttons(choice_c,
                                "binary_buttons",
                                "Binary Buttons",
                                "ON",
                                "OFF",
                                true);

  std::vector<std::pair<int, std::string>> enum_items = {{0, "Low"},
                                                         {1, "Medium"},
                                                         {2, "High"}};
  meta::presets::enum_choice(choice_c,
                             "enum_choice",
                             "Enum Choice",
                             enum_items,
                             1);

  std::vector<std::string> string_choices = {"Red", "Green", "Blue"};
  meta::presets::string_choice(choice_c,
                               "string_choice",
                               "String Choice",
                               string_choices,
                               "Green");

  // 3. GLM presets
#ifdef META_ENABLE_GLM_TYPES
  auto &glm_c = group.add("GLM");
  meta::presets::wavenumber(glm_c,
                            "wavenumber",
                            "Wavenumber",
                            glm::vec2(1.0f, 2.0f),
                            0.0f,
                            5.0f,
                            true);
  meta::presets::range(glm_c,
                       "range",
                       "Range",
                       glm::vec2(0.2f, 0.8f),
                       0.0f,
                       1.0f,
                       true);
  meta::presets::xy(glm_c,
                    "xy",
                    "XY Position",
                    glm::vec2(10.0f, 20.0f),
                    0.0f,
                    50.0f,
                    0.0f,
                    50.0f);
  meta::presets::points(glm_c,
                        "points",
                        "Points Editor",
                        {glm::vec3(5.0f, 5.0f, 0.5f)});
  meta::presets::color(glm_c,
                       "color",
                       "Color Picker",
                       glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
#endif

  // 4. Color Gradient presets
#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
  meta::presets::color_gradient(other_c, "color_gradient", "Color Gradient");
#endif

  // 5. File preset
  meta::presets::file(other_c,
                      "file",
                      "File Selection",
                      "test_file.txt",
                      "*",
                      false);

  // 6. Text preset
  meta::presets::text(other_c, "text", "Text Input", "Hello Meta!");

  // 7. Curve preset
  meta::presets::curve(other_c,
                       "curve",
                       "Curve Editor",
                       {0.0f, 0.5f, 1.0f},
                       0.0f,
                       1.0f);

  // Start Qt Application
  QApplication app(argc, argv);

  meta::qt::ContainerRenderOptions options;
  options.category_policy = meta::qt::CategoryPolicy::CP_MERGED;
  options.snapshot_manager = false;

  meta::qt::MetaWidget *widget = meta::qt::render(group, options);
  widget->setWindowTitle("Meta Presets Qt Test");
  widget->show();

  return app.exec();
}
