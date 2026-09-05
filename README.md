# Meta

Meta is a reflection-inspired metadata framework for C++20, enabling runtime inspection and extension of user-defined attributes. It provides a modular system for building data-driven applications with optional UI backends (Qt/FTXUI) and type-safe extensible attribute handling.

https://github.com/user-attachments/assets/e989df1c-4eca-4f74-a38d-88f4918affb8

# Building Meta (CMake)

## Requirements

* CMake ≥ 3.22
* C++20 compiler (GCC / Clang / MSVC)
* Dependencies:

  * `nlohmann_json`
  * `spdlog`
  * `glm` *(optional)*
  * Qt6 *(optional, UI backend)*
  * FTXUI *(optional, UI backend — stub only)*

## Configure the project

### Basic build (core only)

```bash
cmake -B build
cmake --build build -j
```

This builds:

* Meta core library
* Default optional features (GLM + gradients enabled by default)
* Unit tests (enabled by default)

## CMake options

| Option                             | Description                                     | Default |
| ---------------------------------- | ----------------------------------------------- | ------- |
| `META_ENABLE_TESTS`                | Build unit tests                                | ON      |
| `META_ENABLE_GLM_TYPES`            | Enable GLM type support                         | ON      |
| `META_ENABLE_COLOR_GRADIENT_TYPES` | Enable gradient types                           | ON      |
| `META_ENABLE_FTXUI_UI`             | Enable FTXUI UI backend *(stub implementation)* | OFF     |
| `META_ENABLE_QT_UI`                | Enable Qt UI backend                            | OFF     |

## Gradient library

`meta::ColorGradient` attributes render a `GradientPicker` whose preset grid
shows two sources: host presets installed in attribute metadata
(`keys::ui::presets` → `meta::GradientPresets`) and the user's own
**gradient library** (`meta::GradientLibrary::instance()`), which persists
across projects. From the picker the user can save the current gradient to the
library, import/export gradient files, pin favorites (always shown first) and
sort by name, luminance or hue. Library entries can be renamed, replaced with
the current gradient or deleted from the swatch context menu.

The library autosaves to a JSON file. The Qt picker picks a default location on
first use (`QStandardPaths::AppConfigLocation/gradients.json`, falling back to
`AppDataLocation`); a host can choose its own before any picker exists:

```cpp
auto &library = meta::GradientLibrary::instance();
library.set_path(my_config_dir / "gradients.json");
library.load();
```

Gradient files use the same document for the library, exports and imports:

```json
{
  "format": "meta.gradients",
  "version": 1,
  "gradients": [
    {"name": "Snow", "stops": [{"position": 0.0, "color": [0.78, 0.86, 1.0, 1.0]},
                               {"position": 1.0, "color": [1.0, 1.0, 1.0, 1.0]}]}
  ]
}
```

The reader also accepts a bare array of gradients, a single gradient object,
stops under `"value"` (the `ColorGradient::json_to()` shape) and 0–255 colour
components, so existing per-gradient JSON assets import directly. Imports never
prompt: identical duplicates are skipped and clashing names get a `" (2)"`
suffix.

> **FTXUI backend is currently a stub used for testing and experimental integration only. It is not a complete UI implementation.**

## Example configurations

### Minimal build

```bash
cmake -B build -DMETA_ENABLE_TESTS=OFF
cmake --build build
```

### Qt backend enabled

```bash
cmake -B build -DMETA_ENABLE_QT_UI=ON
cmake --build build
```

### FTXUI stub (for testing only, but could be extended)

```bash
cmake -B build -DMETA_ENABLE_FTXUI_UI=ON
cmake --build build
```

> This backend currently exists only to validate integration points and shared UI abstractions.

## Build output

```text
build/bin/
```

## Design note

Meta is structured as a modular system:

* Core reflection + metadata engine
* Optional UI backends (Qt, FTXUI stub)
* Experimental and extensible architecture

> [!WARNING]
> **Lifetime & GUI Usage Assumptions**
>
> In the current setup, it is assumed that all attributes and `AttributeContainer` instances are created beforehand and their structures are not structurally mutated (e.g. deleting, adding, or replacing attributes) during their usage by the GUI. Swapping out, clearing, or destroying containers while active UI widgets are bound to them will lead to dangling references and undefined behavior/crashes, as view widgets keep references directly to their backing models.

## Extending Meta with a New Built-in Type (Core Source Changes)

To add support for a new type in Meta, follow these steps.

### 1. Register the Type Name

Register the type name in `Meta/include/meta/type/type_name.hpp` to associate the C++ type with a string identifier.

```cpp
namespace meta
{
template <>
struct TypeName<float>
{
    static constexpr std::string_view name = "float";
};
}
```

Alternatively, use the provided macro (outside the `meta` namespace):

```cpp
META_DEFINE_TYPE_NAME(float);
```

### 2. Define the Attribute Traits

In `Meta/include/meta/type/attribute_traits.hpp`, specialize `AttributeTraits<T>` if the generic implementation is not sufficient.

A specialization can provide:

- `to_string()` for string conversion.
- `json_to()` for serialization to `nlohmann::json`.
- `json_from()` for deserialization from `nlohmann::json`.

For example, for `glm::vec2`:

```cpp
template <>
struct AttributeTraits<glm::vec2>
{
    static std::string to_string(const glm::vec2& v)
    {
        return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
    }

    static nlohmann::json json_to(const glm::vec2& v)
    {
        return {{"x", v.x}, {"y", v.y}};
    }

    static glm::vec2 json_from(const nlohmann::json& j)
    {
        return {
            j.at("x").get<float>(),
            j.at("y").get<float>()
        };
    }
};
```

### 3. Register the Type in the Attribute Factory

To enable automatic instantiation during deserialization, register the type in the attribute factory.

For built-in types, add the registration in `Meta/src/serialization/attribute_factory.cpp` inside `register_builtin_types()`:

```cpp
META_REGISTER_ATTRIBUTE_TYPE(float);
```

Alternatively, register the type manually before it is used:

```cpp
meta::register_attribute_type<float>("float");
```

Once these three steps are complete, the type can be serialized, deserialized, and instantiated automatically by the Meta framework.
