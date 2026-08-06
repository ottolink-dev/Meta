/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/presets/choice.hpp"
#include "meta/core/attribute_container.hpp"
#include "meta/metadata/keys.hpp"

namespace meta::presets
{

Attribute<bool> &checkbox(AttributeContainer &c,
                          std::string_view    key,
                          std::string_view    label,
                          bool                value)
{
  auto *a = c.add(std::string(key), value);
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "Checkbox");
  m.add(keys::ui::label, std::string(label));
  return *a;
}

Attribute<bool> &toggle_button(AttributeContainer &c,
                               std::string_view    key,
                               std::string_view    label,
                               bool                value)
{
  auto *a = c.add(std::string(key), value);
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "Toggle");
  m.add(keys::ui::label, std::string(label));
  return *a;
}

Attribute<bool> &binary_buttons(AttributeContainer &c,
                                std::string_view    key,
                                std::string_view    label,
                                std::string_view    label_true,
                                std::string_view    label_false,
                                bool                value)
{
  auto *a = c.add(std::string(key), value);
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "BinaryButtons");
  m.add(keys::ui::label, std::string(label));
  m.add(keys::ui::label_true, std::string(label_true));
  m.add(keys::ui::label_false, std::string(label_false));
  return *a;
}

Attribute<int> &enum_choice(
    AttributeContainer                             &c,
    std::string_view                                key,
    std::string_view                                label,
    const std::vector<std::pair<int, std::string>> &items,
    int                                             value)
{
  auto *a = c.add(std::string(key), value);
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, "EnumComboBox");
  m.add(keys::ui::label, std::string(label));
  m.add(keys::constraints::enum_items, items);
  return *a;
}

Attribute<std::string> &string_choice(AttributeContainer             &c,
                                      std::string_view                key,
                                      std::string_view                label,
                                      const std::vector<std::string> &choices,
                                      std::string                     value,
                                      bool                            use_combo)
{
  auto *a = c.add(std::string(key), std::move(value));
  auto &m = a->metadata();
  m.add(keys::ui::widget_type, use_combo ? "ComboBox" : "ButtonGrid");
  m.add(keys::ui::label, std::string(label));
  m.add(keys::constraints::allowed_values, choices);
  return *a;
}

} // namespace meta::presets
