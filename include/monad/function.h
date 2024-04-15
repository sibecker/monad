// Copyright Stewart 12/02/2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <functional>
#include "monad/task.h"

namespace sib::monad {
    template<typename R, typename... Args, typename... GArgs>
    R operator|(std::function<R(Args...)> const& function, Get<GArgs...>&& get)
    {
        return std::apply(function, std::move(get.args));
    }

    template<typename R, typename... Args, typename... GArgs>
    R operator|(std::function<R(Args...)> const& function, Get<GArgs...> const& get)
    {
        return std::apply(function, get.args);
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
            return [lhs = std::move(lhs), rhs = std::move(rhs)](Args... args){
                try {
                    return lhs(args...);
                } catch (...) {}
                return rhs(args...);
            };
        }
    }

    namespace parallel {
        template<typename R, typename... Args>
        std::function<R(Args...)> operator^(std::function<R(Args...)> lhs, std::function<R(Args...)> rhs)
        {
            return [lhs = std::move(lhs), rhs = std::move(rhs)](Args... args) {
                std::packaged_task<R(Args...)> ltask{std::move(lhs)};
                std::packaged_task<R(Args...)> rtask{std::move(rhs)};

                return (std::move(ltask) ^ std::move(rtask)) | get(std::move(args)...);
            };
        }
    }

    template<typename R, typename... Args, typename Invokable>
    auto operator|(std::function<R(Args...)> function, Then<Invokable> then) -> std::function<std::invoke_result_t<Invokable, R>(Args...)>
    {
        return [function = std::move(function), then = std::move(then)](Args... args){
            return std::move(then)(function(std::move(args)...));
        };
    }

    namespace sequence {
        template<typename T, typename U, typename... LArgs, typename... RArgs>
        std::function<std::tuple<T, U>(LArgs..., RArgs...)>
            operator&(std::function<T(LArgs...)> lhs, std::function<U(RArgs...)> rhs)
        {
            return (std::move(lhs) | then(std::make_tuple<T>)) & (std::move(rhs) | then(std::make_tuple<U>));
        }

        template<typename... Ts, typename U, typename... LArgs, typename... RArgs>
        std::function<std::tuple<Ts..., U>(LArgs..., RArgs...)>
            operator&(std::function<std::tuple<Ts...>(LArgs...)> lhs, std::function<U(RArgs...)> rhs)
        {
            return std::move(lhs) & (std::move(rhs) | then(std::make_tuple<U>));
        }

        template<typename T, typename... Us, typename... LArgs, typename... RArgs>
        std::function<std::tuple<T, Us...>(LArgs..., RArgs...)>
            operator&(std::function<T(LArgs...)> lhs, std::function <std::tuple<Us...>(RArgs...)> rhs)
        {
            return (std::move(lhs) | then(std::make_tuple<T>)) & std::move(rhs);
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

    namespace parallel {
        template<typename T, typename U, typename... LArgs, typename... RArgs>
        std::function<std::tuple<T, U>(LArgs..., RArgs...)>
            operator&(std::function<T(LArgs...)> lhs, std::function<U(RArgs...)> rhs)
        {
            return (std::move(lhs) | then(std::make_tuple<T>)) & (std::move(rhs) | then(std::make_tuple<U>));
        }

        template<typename... Ts, typename U, typename... LArgs, typename... RArgs>
        std::function<std::tuple<Ts..., U>(LArgs..., RArgs...)>
            operator&(std::function<std::tuple<Ts...>(LArgs...)> lhs, std::function<U(RArgs...)> rhs)
        {
            return std::move(lhs) & (std::move(rhs) | then(std::make_tuple<U>));
        }

        template<typename T, typename... Us, typename... LArgs, typename... RArgs>
        std::function<std::tuple<T, Us...>(LArgs..., RArgs...)>
            operator&(std::function<T(LArgs...)> lhs, std::function<std::tuple<Us...>(RArgs...)> rhs)
        {
            return (std::move(lhs) | then(std::make_tuple<T>)) & std::move(rhs);
        }

        template<typename... Ts, typename... Us, typename... LArgs, typename... RArgs>
        std::function<std::tuple<Ts..., Us...>(LArgs..., RArgs...)>
            operator&(std::function<std::tuple<Ts...>(LArgs...)> lhs, std::function<std::tuple<Us...>(RArgs...)> rhs)
        {
            return [lhs = std::move(lhs), rhs = std::move(rhs)](LArgs... largs, RArgs... rargs) {
                auto lfuture = std::async(std::launch::async, lhs, std::move(largs)...);
                auto rfuture = std::async(std::launch::async, rhs, std::move(rargs)...);
                return std::tuple_cat(lfuture.get(), rfuture.get());
            };
        }
    }
}
