/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#pragma once
#include <QToolButton>
#include <QVBoxLayout>

namespace meta::qt
{

class CollapsibleSection : public QWidget
{
  Q_OBJECT

public:
  explicit CollapsibleSection(const QString &title, QWidget *parent = nullptr);
  ~CollapsibleSection() override = default;

  /// Virtual so a design can animate the transition rather than snapping.
  virtual void set_expanded(bool new_state);

  bool is_expanded() const;

  QVBoxLayout *content_layout;

signals:
  void expanded_state_changed(bool new_state);

protected:
  // Protected rather than private so a design can restyle or animate the
  // header and body without this class having to anticipate how.
  QToolButton *toggle_button;
  QWidget     *content;
};

} // namespace meta::qt