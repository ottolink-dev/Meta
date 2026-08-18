/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/core/meta_object.hpp"
#include "meta/core/attribute_container.hpp"

namespace meta
{

AttributeContainer &MetaObject::metadata()
{
  if (!metadata_)
  {
    metadata_ = std::make_unique<AttributeContainer>();
  }
  return *metadata_;
}

const AttributeContainer &MetaObject::metadata() const
{
  if (!metadata_)
  {
    static const AttributeContainer empty_container;
    return empty_container;
  }
  return *metadata_;
}

AttributeContainer &MetaObject::state()
{
  if (!state_)
  {
    state_ = std::make_unique<AttributeContainer>();
  }
  return *state_;
}

const AttributeContainer &MetaObject::state() const
{
  if (!state_)
  {
    static const AttributeContainer empty_container;
    return empty_container;
  }
  return *state_;
}

bool MetaObject::has_metadata() const noexcept
{
  return metadata_ != nullptr && !metadata_->empty();
}

bool MetaObject::has_state() const noexcept
{
  return state_ != nullptr && !state_->empty();
}

} // namespace meta
