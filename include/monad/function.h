// Copyright Stewart 12/02/2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <functional>
#include "monad/task.h"

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
            return function(std::move(outerArgs)...)(std::move(innerArgs)...);
        };
    }

    namespace sequence {
        template<typename R, typename... Args>
        std::function<R(Args...)> operator^(std::function<R(Args...)> lhs, std::function<R(Args...)> rhs)
        {
            return std::function<R(Args...)>{[lhs = std::move(lhs), rhs = std::move(rhs)](Args... args){
                try {
                    return lhs(args...);
                } catch (...) {}
                return rhs(args...);
            }};
        }
    }

    namespace parallel {
        template<typename R, typename... Args>
        std::function<R(Args...)> operator^(std::function<R(Args...)> lhs, std::function<R(Args...)> rhs)
        {
            return std::function<R(Args...)>{[lhs = std::move(lhs), rhs = std::move(rhs)](Args... args) {
                std::packaged_task<R(Args...)> ltask{std::move(lhs)};
                std::packaged_task<R(Args...)> rtask{std::move(rhs)};

                return (std::move(ltask) ^ std::move(rtask)) | get(std::move(args)...);
            }};
        }
    }

    template<typename R, typename... Args, typename Invokable>
    auto operator|(std::function<R(Args...)> function, Then<Invokable> th) -> std::function<std::invoke_result_t<Invokable, R>(Args...)>
    {
        return [function = std::move(function), th = std::move(th)](Args... args){
            return std::move(th)(function(std::move(args)...));
        };
    }

    namespace sequence {
        template<typename... Ls, typename R, typename... LArgs, typename... RArgs>
        std::function<std::tuple<Ls..., R>(LArgs..., RArgs...)>
            operator&(std::function<std::tuple<Ls...>(LArgs...)> lhs, std::function<R(RArgs...)> rhs)
        {
            return [lhs = std::move(lhs), rhs = std::move(rhs) | then(make_tuple)](LArgs... largs, RArgs... rargs) {
                return std::tuple_cat(lhs(std::move(largs)...), rhs(std::move(rargs)...));
            };
        }
    }

    namespace parallel {
        template<typename... Ls, typename R, typename... LArgs, typename... RArgs>
        std::function<std::tuple<Ls..., R>(LArgs..., RArgs...)>
            operator&(std::function<std::tuple<Ls...>(LArgs...)> lhs, std::function<R(RArgs...)> rhs)
        {
            return [lhs = std::move(lhs), rhs = std::move(rhs) | then(make_tuple)](LArgs... largs, RArgs... rargs) {
                auto lfuture = std::async(std::launch::async, lhs, std::move(largs)...);
                auto rfuture = std::async(std::launch::async, rhs, std::move(rargs)...);
                return std::tuple_cat(lfuture.get(), rfuture.get());
            };
        }
    }
}
