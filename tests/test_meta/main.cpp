#include <iostream>

#include "meta.hpp"
#include "meta/core/data_provider.hpp"

struct Vec2
{
  float x;
  float y;
};

namespace meta
{

// -----------------------------------------------------------------------------
// Custom type traits
// -----------------------------------------------------------------------------

template <> struct AttributeTraits<Vec2>
{
  static std::string to_string(const Vec2 &v)
  {
    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
  }

  static nlohmann::json json_to(const Vec2 &v)
  {
    return {{"x", v.x}, {"y", v.y}};
  }

  static Vec2 json_from(const nlohmann::json &j)
  {
    return {j.at("x").get<float>(), j.at("y").get<float>()};
  }
};

} // namespace meta

META_DEFINE_TYPE_NAME(Vec2);

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main()
{
  using namespace meta;

  AttributeContainer container;

  // --- Basic attributes

  container.add("vec2", Vec2{1.f, 0.f}); // custom

  container.add("float", 1.f);
  container.add("double", 1.0);
  container.add("int", 1);
  container.add("bool", true);

  container.add("uint8_t", (uint8_t)8);
  container.add("uint16_t", (uint16_t)16);
  container.add("uint32_t", (uint32_t)32);
  container.add("uint64_t", (uint64_t)64);

  container.add("std::string", "some text");
  container.add("std::filesystem::path", std::filesystem::path("some_path"));
  container.add("std::vector<std::string>", std::vector<std::string>{"a", "b"});
  container.add("std::vector<bool>", std::vector<bool>{true, false});

  {
    std::vector<std::pair<int, std::string>> options = {{0, "Linear"},
                                                        {1, "Cubic"},
                                                        {2, "Bezier"}};
    container.add("std::vector<std::pair<int, std::string>>", options);
  }

#ifdef META_ENABLE_GLM_TYPES
  container.add("glm::vec2", glm::vec2(0.f, 1.f));
  container.add("glm::vec3", glm::vec3(0.f, 1.f, 2.f));
  container.add("glm::vec4", glm::vec4(0.f, 1.f, 2.f, 3.f));

  container.add("glm::ivec2", glm::ivec2(0, 1));
  container.add("glm::ivec3", glm::ivec3(0, 1, 2));
  container.add("glm::ivec4", glm::ivec4(0, 1, 2, 3));

  {
    std::vector<glm::ivec3> values = {glm::ivec3(0, 2, 20),
                                      glm::ivec3(0, 3, 30),
                                      glm::ivec3(0, 4, 40)};
    container.add("std::vector<glm::ivec3>", values);
  }

  {
    std::vector<glm::vec3> values = {glm::vec3(0.1f, 0.2f, 0.1f),
                                     glm::vec3(0.5f, 0.25f, 0.5f),
                                     glm::vec3(0.7f, 0.5f, 1.f)};
    container.add("std::vector<glm::vec3>", values);
  }
#endif

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
  {
    container.add("ColorGradient", ColorGradient());
  }
#endif

#ifdef META_ENABLE_ARRAY_TYPES
  {
    Array arr;
    arr.shape = {2, 3};
    arr.vector = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f};
    container.add("Array", arr);
  }
#endif

  // --- Metadata example

  {
    auto *attr = container.add("float_with_metadata", -1.f);

    auto &meta = attr->metadata();
    meta.add("vmin", -10.f);
    meta.add("vmax", 10.f);
    meta.add("scale", "linear");
  }

  // --- Print attributes

  for (const auto &[name, attr] : container)
  {
    std::cout << name << " : " << attr->to_string() << '\n';
  }

  std::cout << "\nSerialized container:\n";
  std::cout << container.json_to().dump(4) << "\n";

  // --- Pass-through

  std::cout << "\nPass through, container level accessor(s):\n";
  std::cout << "float value: " << container.value<float>("float") << "\n";

  // ---------------------------------------------------------------------------
  // Deserialization test
  // ---------------------------------------------------------------------------

  AttributeContainer container2;

  // "built-in" types for serialization
  meta::register_builtin_types();

  // any specific class needs to be registered to the attribute
  // factory to allow deserialization
  register_attribute_type<Vec2>("Vec2");

  container2.json_from(container.json_to());

  std::cout << "\nDeserialized container:\n";
  std::cout << container2.json_to().dump(4) << "\n";

  // ---------------------------------------------------------------------------
  // Groups
  // ---------------------------------------------------------------------------

  ContainerGroup group;

  // Create multiple "views" / contexts
  auto &node_settings = group.add("node_settings");
  auto &ui_settings = group.add("ui_settings");
  auto &debug_settings = group.add("debug_settings");

  // Fill node settings
  node_settings.add("threshold", 0.5f);
  node_settings.add("iterations", 8);
  node_settings.add("active", true);

  // Fill UI settings
  ui_settings.add("theme", std::string("dark"));
  ui_settings.add("font_size", 14.f);
  ui_settings.add("show_grid", true);

  // Fill debug settings
  debug_settings.add("log_level", 2);
  debug_settings.add("wireframe", false);
  debug_settings.add("draw_bounds", true);

  // ---------------------------------------------------------------------------
  // Switch active container (runtime UI context change)
  // ---------------------------------------------------------------------------

  group.set_current("node_settings");

  std::cout << "\nCurrent (node_settings):\n";
  std::cout << "threshold = " << group.current().value<float>("threshold")
            << "\n";
  std::cout << "iterations = " << group.current().value<int>("iterations")
            << "\n";

  // Now switch to UI settings (same GUI, different model)
  group.set_current("ui_settings");

  std::cout << "\nCurrent (ui_settings):\n";
  std::cout << "theme = " << group.current().value<std::string>("theme")
            << "\n";
  std::cout << "font_size = " << group.current().value<float>("font_size")
            << "\n";

  // Switch again to debug context
  group.set_current("debug_settings");

  std::cout << "\nCurrent (debug_settings):\n";
  std::cout << "log_level = " << group.current().value<int>("log_level")
            << "\n";
  std::cout << "wireframe = " << group.current().value<bool>("wireframe")
            << "\n";

  // ---------------------------------------------------------------------------
  // UI-facing usage pattern (important part)
  // ---------------------------------------------------------------------------
  //
  // This is what your widget system would actually use:
  // it only sees "group.current()" as the active model.

  AttributeContainer &ui = group.current();

  std::cout << "\nUI-facing access:\n";
  std::cout << "draw_bounds = " << ui.value<bool>("draw_bounds") << "\n";

  {
    struct TestData
    {
    };

    // Attribute<DataProvider> compiles, to_string is the no-op sentinel
    meta::AttributeContainer c;
    bool                     called = false;
    c.add<meta::DataProvider>("p",
                              meta::DataProvider{[&]
                                                 {
                                                   called = true;
                                                   return TestData{};
                                                 }});
    auto *a = c.find("p");
    assert(a && a->to_string() == "<data_provider>");

    // the stored provider is callable
    auto *typed = a->try_cast<meta::Attribute<meta::DataProvider>>();
    assert(typed && typed->value());
    meta::Any res = typed->value()();
    assert(res.has_value());
    assert(res.get<TestData>() != nullptr);
    assert(called);

    std::cout << "[data_provider] core OK" << std::endl;
  }

  {
    struct TestData
    {
    };
    meta::AttributeContainer c;
    c.add<int>("keep", 7);
    c.add<meta::DataProvider>("skip",
                              meta::DataProvider{[] { return TestData{}; }});
    auto j = c.json_to();
    assert(j.contains("keep"));
    assert(!j.contains("skip")); // DataProvider omitted from serialization
    std::cout << "[data_provider] serialize-skip OK" << std::endl;
  }

#ifdef META_ENABLE_COLOR_GRADIENT_TYPES
  {
    // gradient presets live in attribute metadata, not in the value: they are
    // host configuration, must not be serialized, and must survive json_from
    meta::AttributeContainer c;
    auto                    *a = c.add("g", meta::ColorGradient());
    a->metadata().add(meta::keys::ui::presets,
                      meta::GradientPresets{{{"Reds",
                                              {{0.f, {1.f, 0.f, 0.f, 1.f}},
                                               {1.f, {1.f, 1.f, 1.f, 1.f}}}}}});

    auto j = c.json_to();
    assert(!j["g"]["metadata"].contains(
        meta::keys::ui::presets)); // not serialized

    // reload in place: value updates, installed presets untouched
    c.json_from(j);
    auto *pp = a->metadata().try_value<meta::GradientPresets>(
        meta::keys::ui::presets);
    assert(pp && pp->presets.size() == 1 && pp->presets[0].name == "Reds");

    // decode into a fresh container: no phantom presets appear
    meta::AttributeContainer c2;
    c2.json_from(j);
    auto *a2 = c2.find("g");
    assert(a2 && !a2->metadata().contains(meta::keys::ui::presets));
    std::cout << "[gradient_presets] metadata round-trip OK" << std::endl;
  }
#endif

#ifdef META_ENABLE_ARRAY_TYPES
  {
    meta::AttributeContainer c;
    Array                    arr;
    arr.shape = {3, 2};
    arr.vector = {10.f, 20.f, 30.f, 40.f, 50.f, 60.f};
    c.add("arr", arr);

    auto j = c.json_to();
    assert(j["arr"]["value"]["vector"].is_binary());

    meta::AttributeContainer c2;
    c2.json_from(j);
    auto *a2 = c2.find("arr");
    assert(a2);
    auto const &decoded = c2.value<meta::Array>("arr");
    assert(decoded.shape.x == 3 && decoded.shape.y == 2);
    assert(decoded.vector.size() == 6);
    assert(decoded.vector[2] == 30.f);

    std::cout << "[array] binary round-trip OK" << std::endl;
  }
#endif

  // --- presets::compat smoke
  {
    meta::AttributeContainer pc;
    auto &f = meta::presets::slider_float(pc, "f", "Float", 0.5f, 0.f, 1.f);
    assert(f.value() == 0.5f);
    assert(pc.value<float>("f") == 0.5f);
    assert(f.metadata().value<std::string>(meta::keys::ui::widget_type) ==
           "SliderFloat");
    assert(f.metadata().value<float>(meta::keys::constraints::max) == 1.f);

    std::vector<std::pair<int, std::string>> items = {{0, "a"}, {1, "b"}};
    auto &e = meta::presets::enum_choice(pc, "e", "Enum", items, 1);
    assert(e.value() == 1);

    auto &r =
        meta::presets::range(pc, "r", "Range", {0.f, 1.f}, -1.f, 2.f, false);
    assert(r.metadata().value<bool>("ui.active") == false);

    auto &ch = meta::presets::string_choice(pc, "c", "Choice", {"x", "y"}, "x");
    assert(ch.value() == "x");

    auto &sf = meta::presets::file(pc,
                                   "p",
                                   "File",
                                   "out.png",
                                   "PNG (*.png)",
                                   true);
    assert(sf.metadata().value<std::string>(meta::keys::ui::widget_type) ==
           "SaveFile");

    std::cout << "presets::compat smoke OK" << std::endl;
  }

  // --- set_insertion_order
  {
    meta::AttributeContainer oc;
    oc.add("a", 1);
    oc.add("b", 2);
    oc.add("c", 3);
    assert(oc.set_insertion_order({"c", "a", "b"}));
    assert(oc.insertion_order() == (std::vector<std::string>{"c", "a", "b"}));
    assert(!oc.set_insertion_order({"c", "a"})); // wrong size
    assert(oc.insertion_order() == (std::vector<std::string>{"c", "a", "b"}));
    assert(!oc.set_insertion_order({"c", "a", "zzz"})); // unknown key
    assert(oc.insertion_order() == (std::vector<std::string>{"c", "a", "b"}));
    assert(!oc.set_insertion_order({"c", "c", "b"})); // duplicate key
    assert(oc.insertion_order() == (std::vector<std::string>{"c", "a", "b"}));
    std::cout << "set_insertion_order OK" << std::endl;
  }

  // --- Event teardown safety: a connection outliving its Event must not UAF
  {
    meta::EventConnection conn;
    {
      meta::Event<int> ev;
      conn = ev.subscribe([](int) {});
    } // ev destroyed; conn still alive
    // conn's destructor (end of this block) must NOT touch the freed Event.
    std::cout << "event teardown-safety OK" << std::endl;
  }

  return 0;
}
