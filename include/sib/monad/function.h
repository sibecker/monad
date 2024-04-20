// Copyright Stewart 12/02/2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "sib/monad/task.h"
#include <functional>

namespace sib::monad {

template<typename R, typename... Args, typename... GArgs>
R operator|(std::function<R(Args...)> const& function, Get<GArgs...>&& g)
{
    return std::apply(function, std::move(g.args));
}

template<typename R, typename... Args, typename... GArgs>
R operator|(std::function<R(Args...)> const& function, Get<GArgs...> const& g)
{
    return std::apply(function, g.args);
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
        return function | get(std::move(outerArgs)...) | get(std::move(innerArgs)...);
    };
}

template<typename R, typename... Args, typename Invocable>
auto operator|(std::function<R(Args...)> function, Then<Invocable> f)
{
    using Result = decltype(std::invoke(f, std::move(function) | get()));
    return std::function<Result(Args...)>{
        [function = std::move(function), f = std::move(f)](Args... args) {
            return f(function(std::move(args)...));
        }
    } | flatten();
}

template<typename R, typename... Args>
When<std::function<R(Args...)>> operator^(When<std::function<R(Args...)>> lhs, std::function<R(Args...)> rhs)
{
    return {lhs.manner,
            std::function<R(Args...)>{[lhs = std::move(lhs), rhs = std::move(rhs)](Args... args) {
                std::packaged_task<R(Args...)> ltask{std::move(lhs.value)};
                std::packaged_task<R(Args...)> rtask{std::move(rhs)};
                return (lhs.manner ^ std::move(ltask) ^ std::move(rtask)) | get(std::move(args)...);
            }}
    };
}

template<typename... Ls, typename R, typename... LArgs, typename... RArgs>
When<std::function<std::tuple<Ls..., R>(LArgs..., RArgs...)>>
    operator&(When<std::function<std::tuple<Ls...>(LArgs...)>> lhs, std::function<R(RArgs...)> rhs)
{
    auto lambda = [lhs = std::move(lhs), rhs = std::move(rhs)](LArgs... largs, RArgs... rargs) -> std::tuple<Ls..., R> {
        std::packaged_task<std::tuple<Ls...>(LArgs...)> ltask{std::move(lhs.value)};
        std::packaged_task<R(RArgs...)> rtask{std::move(rhs)};
        auto combined_task = when_any(lhs.manner, std::move(ltask)) & std::move(rtask);
        return std::move(combined_task) | get(std::move(largs)..., std::move(rargs)...);
    };
    return {lhs.manner,
        std::function<std::tuple<Ls..., R>(LArgs..., RArgs...)>{lambda}
    };
}

}
