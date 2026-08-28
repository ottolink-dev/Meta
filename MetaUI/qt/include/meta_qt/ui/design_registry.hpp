/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <functional>
#include <map>
#include <string>
#include <typeindex>
#include <vector>

#include <QLayout>

#include "meta/core/abstract_attribute.hpp"

#include "meta_qt/meta_widget.hpp"
#include "meta_qt/ui/binding.hpp"
#include "meta_qt/ui/control.hpp"

namespace meta::qt
{

/// Builds a fully bound row for one attribute, or nullptr if it cannot.
using RowFactory = std::function<
    MetaWidget *(AbstractAttribute &, const RowContext &, QWidget *)>;

/// Matches any widget_type for a given C++ type.
inline constexpr char kAnyWidgetType[] = "*";

/** @brief Construct-and-bind a control of type `ControlT` for attribute type `T`.
 *
 * The uniform control constructor is
 * `ControlT(Attribute<T> &, const RowContext &, QWidget *)`. A control reads its
 * own metadata; binding is identical for every control of a type and lives in
 * bind().
 *
 * A control must also provide `static bool can_render(const Attribute<T> &)`.
 * Returning false declines the attribute and the row falls back to the stock
 * renderer -- the required escape hatch for metadata a design cannot honour,
 * such as a rail with no min/max to span. Making it part of the contract means
 * a control cannot forget to check and silently render a [0, 0] range instead.
 */
template <class T, class ControlT> RowFactory make_row_factory()
{
  return [](AbstractAttribute &abstract_attr,
            const RowContext  &ctx,
            QWidget           *parent) -> MetaWidget *
  {
    auto &attr = static_cast<Attribute<T> &>(abstract_attr);

    if (!ControlT::can_render(attr)) return nullptr;

    MetaWidget *host = make_meta_widget_vbox(parent);
    auto       *control = new ControlT(attr, ctx, host);

    host->layout()->addWidget(control);
    bind<T>(attr, *control, *host);

    return host;
  };
}

/** @brief Maps (design, C++ type, widget_type) to a row factory.
 *
 * This is what makes a second visual design a registration rather than an edit
 * to a dispatch function. The stock renderer resolves type and widget_type
 * through two hardcoded if-chains, so every new design or type meant editing
 * shared code; here they are table entries.
 *
 * Lookup order for a given design:
 *   1. exact (type, widget_type)
 *   2. (type, "*")
 *   3. miss -- the caller falls back to the stock meta::qt::render()
 *
 * That fallback is deliberate and load-bearing: it is what lets a design cover
 * three widget types and still leave a completely usable panel.
 */
class DesignRegistry
{
public:
  static DesignRegistry &instance();

  /// Register a factory. Replaces any existing entry for the same key.
  void add(const std::string &design,
           std::type_index    type,
           const std::string &widget_type,
           RowFactory         factory);

  /// Convenience wrapper around add() + make_row_factory().
  template <class T, class ControlT>
  void register_control(const std::string &design, const std::string &widget_type)
  {
    add(design, std::type_index(typeid(T)), widget_type, make_row_factory<T, ControlT>());
  }

  /** @brief Build a row for `p_attr` using `design`.
   *
   * Returns nullptr when the design has nothing registered for this attribute,
   * which the caller should treat as "use the stock renderer" rather than as an
   * error. See render_row().
   */
  MetaWidget *render(AbstractAttribute *p_attr,
                     const std::string &design,
                     const RowContext  &ctx,
                     QWidget           *parent = nullptr) const;

  bool has_design(const std::string &design) const;

  /// Registered design names, for a settings UI.
  std::vector<std::string> designs() const;

private:
  DesignRegistry() = default;

  using Key = std::pair<std::type_index, std::string>;

  std::map<std::string, std::map<Key, RowFactory>> factories_;
};

/** @brief Render one attribute, falling back to the stock renderer on a miss.
 *
 * The single entry point a panel should call. Keeps the fallback in one place
 * so a partially ported design cannot leave holes in a panel.
 */
MetaWidget *render_row(AbstractAttribute *p_attr,
                       const std::string &design,
                       const RowContext  &ctx,
                       QWidget           *parent = nullptr);

} // namespace meta::qt
