// Copyright Stewart 11/02/2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "monad/monad.h"
#include <optional>

namespace sib::monad {

template<typename T>
T& operator|(std::optional<T>& opt, Get)
{
    return opt.value();
}

template<typename T>
T const& operator|(std::optional<T> const& opt, Get)
{
    return opt.value();
}

template<typename T>
T&& operator|(std::optional<T>&& opt, Get)
{
    return std::move(opt).value();
}

template<typename T>
T const&& operator|(std::optional<T> const&& opt, Get)
{
    return std::move(opt).value();
}

}
