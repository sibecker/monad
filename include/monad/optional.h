// Copyright Stewart 11/02/2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "monad/monad.h"
#include <optional>

namespace sib::monad {

template<typename T>
T& operator|(std::optional<T>& opt, Get) {
    return opt.value();
}

template<typename T>
T const& operator|(std::optional<T> const& opt, Get) {
    return opt.value();
}

template<typename T>
T&& operator|(std::optional<T>&& opt, Get) {
    return std::move(opt).value();
}

template<typename T>
T const&& operator|(std::optional<T> const&& opt, Get) {
    return std::move(opt).value();
}

template<typename T>
std::optional<T> operator|(std::optional<T> opt, Flatten) {
    return std::move(opt);
}

template<typename T>
std::optional<T> operator|(std::optional<std::optional<T>> opt, Flatten) {
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

template<typename T, typename... Tail>
std::optional<T> when_any(std::optional<T> const& head, Tail&&... tail)
{
    return (std::move(head) ^ ... ^ tail);
}

template<typename T, typename... Tail>
std::optional<T> when_any(std::optional<T>&& head, Tail&&... tail)
{
    return (std::move(head) ^ ... ^ tail);
}

template<typename T, typename Callable>
auto operator|(std::optional<T> const& opt, Then<Callable> const& f)
{
    using Result = decltype(std::invoke(f, *opt));
    using Flattened = decltype(std::optional<Result>{} | flatten);
    return opt ?
        std::optional<Result>{std::invoke(f, *opt)} | flatten :
        Flattened{std::nullopt};
}

template<typename T, typename Callable>
auto operator|(std::optional<T>&& opt, Then<Callable> const& f)
{
    using Result = decltype(std::invoke(f, *std::move(opt)));
    using Flattened = decltype(std::optional<Result>{} | flatten);
    return opt ?
           std::optional<Result>{std::invoke(f, *std::move(opt))} | flatten :
           Flattened{std::nullopt};
}

template<typename T, typename Callable>
auto operator|(std::optional<T> const& opt, Then<Callable>&& f)
{
    using Result = decltype(std::invoke(std::move(f), *opt));
    using Flattened = decltype(std::optional<Result>{} | flatten);
    return opt ?
           std::optional<Result>{std::invoke(std::move(f), *opt)} | flatten :
           Flattened{std::nullopt};
}

template<typename T, typename Callable>
auto operator|(std::optional<T>&& opt, Then<Callable>&& f)
{
    using Result = decltype(std::invoke(std::move(f), *std::move(opt)));
    using Flattened = decltype(std::optional<Result>{} | flatten);
    return opt ?
           std::optional<Result>{std::invoke(std::move(f), *std::move(opt))} | flatten :
           Flattened{std::nullopt};
}

}
