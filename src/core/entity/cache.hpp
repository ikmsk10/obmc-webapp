// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __CACHE_H__
#define __CACHE_H__

#include <core/exceptions.hpp>
#include <logger/logger.hpp>
#include <core/entity/entity.hpp>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace app
{
namespace entity
{

class ICache
{
  public:
    virtual ~ICache() noexcept = default;
};

template <class TCacheTarget>
class Cache : public ICache
{
  public:
    Cache(const Cache&) = delete;
    Cache& operator=(const Cache& old) = delete;
    Cache(Cache&&) = delete;
    Cache& operator=(Cache&& old) = delete;

    explicit Cache() = default;
    ~Cache() noexcept override = default;
};

} // namespace entity
} // namespace app

#endif // __CACHE_H__
