/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
   Public License. The full license is in the file LICENSE, distributed with
   this software. */
#include "meta/core/meta_object.hpp"
#include "meta/core/attribute_container.hpp"

namespace meta
{

MetaObject::MetaObject()
    : metadata_(std::make_unique<AttributeContainer>()),
      state_(std::make_unique<AttributeContainer>())
{
}

AttributeContainer &MetaObject::metadata() { return *metadata_; }

const AttributeContainer &MetaObject::metadata() const { return *metadata_; }

AttributeContainer &MetaObject::state() { return *state_; }

const AttributeContainer &MetaObject::state() const { return *state_; }

} // namespace meta
