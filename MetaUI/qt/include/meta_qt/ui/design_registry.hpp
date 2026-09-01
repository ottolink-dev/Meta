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
 * Returning false declines the attribute and resolution continues down the
 * design's fallback chain -- the required escape hatch for metadata a design
 * cannot honour, such as a rail with no min/max to span. Making it part of the
 * contract means a control cannot forget to check and silently render a
 * [0, 0] range instead.
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
    bind_control<T>(attr, *control, *host);

    return host;
  };
}

/** @brief Maps (design, C++ type, widget_type) to a row factory.
 *
 * This is what makes a visual design a registration rather than an edit to a
 * dispatch function. Dispatch used to resolve type and widget_type through two
 * hardcoded if-chains, so every new design or type meant editing shared code;
 * here they are table entries.
 *
 * Every design is a flavour at the same level -- "stock" is registered like any
 * other (see designs/stock) and holds no special position in the dispatcher.
 *
 * Resolution for a given design, then repeated down its fallback chain:
 *   1. exact (type, widget_type)
 *   2. (type, "*")
 *   3. otherwise continue with the fallback design, if one is set
 *
 * A factory returning nullptr counts as a miss, so a control declining an
 * attribute via can_render() resumes the same walk.
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

  /** @brief Make `design` fall back to `fallback` for anything it lacks.
   *
   * Flavours sit at the same level -- there is no privileged design in the
   * dispatcher. But a design under construction covers only some widget types,
   * and a panel that renders two rows and drops the rest is not usable, so the
   * chain is expressed as *data* rather than as a special case in code.
   *
   * Set "industrial" -> "stock" and an unported widget type renders stock;
   * leave it unset and an unregistered type renders nothing. Cycles are
   * ignored rather than followed.
   */
  void set_fallback(const std::string &design, const std::string &fallback);

  /// Convenience wrapper around add() + make_row_factory().
  template <class T, class ControlT>
  void register_control(const std::string &design, const std::string &widget_type)
  {
    add(design, std::type_index(typeid(T)), widget_type, make_row_factory<T, ControlT>());
  }

  /** @brief Build a row for `p_attr` using `design`, following its fallbacks.
   *
   * Returns nullptr when neither `design` nor anything in its fallback chain
   * has a factory for this attribute, or when every candidate declined it via
   * can_render().
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
  std::map<std::string, std::string>               fallbacks_;
};

/** @brief Render one attribute using `design`.
 *
 * The single entry point a panel should call. Thin by design: dispatch lives
 * entirely in DesignRegistry, fallback chain included, so there is no hardcoded
 * renderer sitting behind it.
 */
MetaWidget *render_row(AbstractAttribute *p_attr,
                       const std::string &design,
                       const RowContext  &ctx,
                       QWidget           *parent = nullptr);

} // namespace meta::qt
