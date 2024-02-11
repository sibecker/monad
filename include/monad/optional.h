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
std::optional<T> when_any(std::initializer_list<std::optional<T>> list)
{
    auto const it = std::find_if(std::begin(list), std::end(list),
                                 [](auto const &opt){ return opt.has_value(); });
    return it == std::end(list) ? std::optional<T>{} : std::move(*it);
}

template<typename T, typename... Tail>
std::optional<T> when_any(std::optional<T> head, Tail&&... tail)
{
    return when_any({std::move(head), std::forward<Tail>(tail)...});
}

}
