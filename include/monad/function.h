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

    template<typename R, typename... Args, typename Invokable>
    auto operator|(std::function<R(Args...)> function, Then<Invokable> then) -> std::function<std::invoke_result_t<Invokable, R>(Args...)>
    {
        return [function = std::move(function), then = std::move(then)](Args... args){
            return std::move(then)(function(std::move(args)...));
        };
    }

    template<typename T, typename U, typename... LArgs, typename... RArgs>
    std::function<std::tuple<T, U>(LArgs..., RArgs...)>
        operator&(std::function<T(LArgs...)> lhs, std::function<U(RArgs...)> rhs)
    {
        return [lhs = std::move(lhs), rhs = std::move(rhs)](LArgs... largs, RArgs... rargs) {
            return std::tuple<T, U>{lhs(std::move(largs)...), rhs(std::move(rargs)...)};
        };
    }

    template<typename... Ts, typename U, typename... LArgs, typename... RArgs>
    std::function<std::tuple<Ts..., U>(LArgs..., RArgs...)>
        operator&(std::function<std::tuple<Ts...>(LArgs...)> lhs, std::function<U(RArgs...)> rhs)
    {
        return [lhs = std::move(lhs), rhs = std::move(rhs)](LArgs... largs, RArgs... rargs) {
            return std::tuple_cat(lhs(std::move(largs)...), std::tuple<U>{rhs(std::move(rargs)...)});
        };
    }

    template<typename T, typename... Us, typename... LArgs, typename... RArgs>
    std::function<std::tuple<T, Us...>(LArgs..., RArgs...)>
        operator&(std::function<T(LArgs...)> lhs, std::function<std::tuple<Us...>(RArgs...)> rhs)
    {
        return [lhs = std::move(lhs), rhs = std::move(rhs)](LArgs... largs, RArgs... rargs) {
            return std::tuple_cat(std::tuple<T>{lhs(std::move(largs)...)}, rhs(std::move(rargs)...));
        };
    }

    template<typename... Ts, typename... Us, typename... LArgs, typename... RArgs>
    std::function<std::tuple<Ts..., Us...>(LArgs..., RArgs...)>
        operator&(std::function<std::tuple<Ts...>(LArgs...)> lhs, std::function<std::tuple<Us...>(RArgs...)> rhs)
    {
        return [lhs = std::move(lhs), rhs = std::move(rhs)](LArgs... largs, RArgs... rargs) {
            return std::tuple_cat(lhs(std::move(largs)...), rhs(std::move(rargs)...));
        };
    }
}
