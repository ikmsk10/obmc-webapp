// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __HELPERS_COMPARES_H__
#define __HELPERS_COMPARES_H__

#include <logger/logger.hpp>

#include <core/entity/entity.hpp>

#include <string>
#include <memory>

namespace app
{
namespace helpers
{
namespace compares
{

using namespace app::entity;

static bool relationCompareEqual(
    const IEntity::IEntityMember::InstancePtr& instance,
    const IEntity::IEntityMember::IInstance::FieldType& value)
{
    return instance->getValue() == value;
}

static bool relationUnconditional(
    const IEntity::IEntityMember::InstancePtr&,
    const IEntity::IEntityMember::IInstance::FieldType&)
{
    return true;
}

} // namespace compares
} // namespace helpers
} // namespace app

#endif // __HELPERS_COMPARES_H__
