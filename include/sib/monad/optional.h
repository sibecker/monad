// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "sib/monad/monad.h"
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

template<typename T, typename Invocable>
auto operator|(std::optional<T> const& opt, Then<Invocable> const& f)
{
    using Result = decltype(std::invoke(f, *opt));
    using Flattened = decltype(std::optional<Result>{} | flatten());
    return opt ?
        std::optional<Result>{std::invoke(f, *opt)} | flatten() :
        Flattened{std::nullopt};
}

template<typename T, typename Invocable>
auto operator|(std::optional<T>&& opt, Then<Invocable> const& f)
{
    using Result = decltype(std::invoke(f, std::move(opt) | get()));
    using Flattened = decltype(std::optional<Result>{} | flatten());
    return opt ?
           std::optional<Result>{std::invoke(f, std::move(opt) | get())} | flatten() :
           Flattened{std::nullopt};
}

template<typename T, typename Invocable>
auto operator|(std::optional<T> const& opt, Then<Invocable>&& f)
{
    using Result = decltype(std::invoke(std::move(f), *opt));
    using Flattened = decltype(std::optional<Result>{} | flatten());
    return opt ?
           std::optional<Result>{std::invoke(std::move(f), *opt)} | flatten() :
           Flattened{std::nullopt};
}

template<typename T, typename Invocable>
auto operator|(std::optional<T>&& opt, Then<Invocable>&& f)
{
    using Result = decltype(std::invoke(std::move(f), *std::move(opt)));
    using Flattened = decltype(std::optional<Result>{} | flatten());
    return opt ?
           std::optional<Result>{std::invoke(std::move(f), *std::move(opt))} | flatten() :
           Flattened{std::nullopt};
}

template<typename T>
When<std::optional<T>> operator^(When<std::optional<T>> const& lhs, std::optional<T> const& rhs)
{
    return {lhs.manner, lhs.value ? lhs.value : rhs};
}

template<typename T>
When<std::optional<T>> operator^(When<std::optional<T>>&& lhs, std::optional<T> const& rhs)
{
    return {lhs.manner, lhs.value ? std::move(lhs.value) : rhs};
}

template<typename T>
When<std::optional<T>> operator^(When<std::optional<T> const>& lhs, std::optional<T>&& rhs)
{
    return {lhs.manner, lhs ? lhs : std::move(rhs)};
}

template<typename T>
When<std::optional<T>> operator^(When<std::optional<T>>&& lhs, std::optional<T>&& rhs)
{
    return {lhs.manner, lhs ? std::move(lhs) : std::move(rhs)};
}

template<typename... Ls, typename R>
When<std::optional<std::tuple<Ls..., R>>> operator&(When<std::optional<std::tuple<Ls...>>> lhs, std::optional<R> rhs)
{
    return when_any(lhs.manner, lhs.value && rhs ?
        std::optional<std::tuple<Ls..., R>>{std::tuple_cat(lhs | get(), std::move(rhs) | then(make_tuple) | get())} :
        std::optional<std::tuple<Ls..., R>>{}
    );
}

}
