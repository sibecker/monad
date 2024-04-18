// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "monad/monad.h"
#include <optional>

namespace sib::monad {

template<typename T>
T operator|(std::optional<T> const& opt, Get<>)
{
    return opt.value();
}

template<typename T>
T operator|(std::optional<T>&& opt, Get<>)
{
    return std::move(opt).value();
}

template<typename T>
std::optional<T> operator|(std::optional<T> opt, Flatten)
{
    return std::move(opt);
}

template<typename T>
std::optional<T> operator|(std::optional<std::optional<T>> opt, Flatten)
{
    return std::move(opt).value_or(std::nullopt);
}

template<typename T>
std::optional<T> operator^(std::optional<T> const& lhs, std::optional<T> const& rhs)
{
    return lhs ? lhs : rhs;
}

template<typename T>
std::optional<T> operator^(std::optional<T>&& lhs, std::optional<T> const& rhs)
{
    return lhs ? std::move(lhs) : rhs;
}

template<typename T>
std::optional<T> operator^(std::optional<T> const& lhs, std::optional<T>&& rhs)
{
    return lhs ? lhs : std::move(rhs);
}

template<typename T>
std::optional<T> operator^(std::optional<T>&& lhs, std::optional<T>&& rhs)
{
    return lhs ? std::move(lhs) : std::move(rhs);
}

template<typename T, typename Invokable>
auto operator|(std::optional<T> const& opt, Then<Invokable> const& f)
{
    using Result = decltype(std::invoke(f, *opt));
    using Flattened = decltype(std::optional<Result>{} | flatten());
    return opt ?
        std::optional<Result>{std::invoke(f, *opt)} | flatten() :
        Flattened{std::nullopt};
}

template<typename T, typename Invokable>
auto operator|(std::optional<T>&& opt, Then<Invokable> const& f)
{
    using Result = decltype(std::invoke(f, *std::move(opt)));
    using Flattened = decltype(std::optional<Result>{} | flatten());
    return opt ?
           std::optional<Result>{std::invoke(f, *std::move(opt))} | flatten() :
           Flattened{std::nullopt};
}

template<typename T, typename Invokable>
auto operator|(std::optional<T> const& opt, Then<Invokable>&& f)
{
    using Result = decltype(std::invoke(std::move(f), *opt));
    using Flattened = decltype(std::optional<Result>{} | flatten());
    return opt ?
           std::optional<Result>{std::invoke(std::move(f), *opt)} | flatten() :
           Flattened{std::nullopt};
}

template<typename T, typename Invokable>
auto operator|(std::optional<T>&& opt, Then<Invokable>&& f)
{
    using Result = decltype(std::invoke(std::move(f), *std::move(opt)));
    using Flattened = decltype(std::optional<Result>{} | flatten());
    return opt ?
           std::optional<Result>{std::invoke(std::move(f), *std::move(opt))} | flatten() :
           Flattened{std::nullopt};
}

template<typename... Ls, typename R>
std::optional<std::tuple<Ls..., R>> operator&(std::optional<std::tuple<Ls...>> lhs, std::optional<R> rhs)
{
    return lhs && rhs ?
        std::optional<std::tuple<Ls..., R>>{std::tuple_cat(lhs | get(), std::move(rhs) | then(make_tuple) | get())} :
        std::optional<std::tuple<Ls..., R>>{};
}

}
