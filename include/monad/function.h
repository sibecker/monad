// Copyright Stewart 12/02/2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <functional>
#include "monad/monad.h"

namespace sib::monad {
    template<typename R>
    R operator|(std::function<R()> const& function, Get)
    {
        return function();
    }

    template<typename Signature>
    std::function<Signature> operator|(std::function<Signature> function, Flatten)
    {
        return std::move(function);
    }

    template<typename R, typename... InnerArgs, typename... OuterArgs>
    std::function<R(InnerArgs..., OuterArgs...)> operator|(std::function<std::function<R(InnerArgs...)>(OuterArgs...)> function, Flatten)
    {
        return [function](InnerArgs... innerArgs, OuterArgs... outerArgs) {
            return function(std::move(outerArgs)...)(std::move(innerArgs)...);
        };
    }

    template<typename Signature>
    std::function<Signature> operator^(std::function<Signature> lhs, std::function<Signature> const&)
    {
        return std::move(lhs);
    }
}
