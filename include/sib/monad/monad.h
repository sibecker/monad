// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <tuple>
#include <utility>

namespace sib::monad {

static inline constexpr struct {
    template<typename T>
    auto operator()(T&& value) const
    {
        return std::forward<T>(value);
    }
} identity;

static inline constexpr struct {
    template<typename T>
    auto operator()(T&& value) const
    {
        return std::make_tuple(std::forward<T>(value));
    }
} make_tuple;

template<typename... Args>
struct Get
{
    std::tuple<Args...> args;
};
static inline constexpr struct {
    template<typename... Args>
    Get<Args...> operator()(Args&& ... args) const
    {
        return Get<Args...>{std::forward<Args>(args)...};
    }
} get;

class Flatten {};
static inline constexpr struct {
    Flatten operator()() const
    {
        return Flatten{};
    }
} flatten;

template<typename Invocable>
class Then
{
private:
    Invocable invocable;

public:
    explicit Then(Invocable callable)
        : invocable{std::move(callable)}
    {}

    template<typename... Args>
    auto operator()(Args&&... args) const &
    {
        return std::invoke(invocable, std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto operator()(Args&&... args) &&
    {
        return std::invoke(std::move(invocable), std::forward<Args>(args)...);
    }
};
static constexpr inline struct {
    template<typename Invocable>
    Then<std::decay_t<Invocable>> operator()(Invocable&& invocable) const
    {
        return Then<std::decay_t<Invocable>>{std::forward<Invocable>(invocable)};
    }
} then;

template<typename Applicable>
class Apply
{
private:
    Applicable applicable;

public:
    explicit Apply(Applicable callable)
            : applicable{std::move(callable)}
    {}

    template<typename... Args>
    auto operator()(Args&&... args) const &
    {
        return std::apply(applicable, std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto operator()(Args&&... args) &&
    {
        return std::apply(std::move(applicable), std::forward<Args>(args)...);
    }
};
static constexpr inline struct {
    template<typename Applicable>
    Apply<std::decay_t<Applicable>> operator()(Applicable&& applicable) const
    {
        return Apply<std::decay_t<Applicable>>{std::forward<Applicable>(applicable)};
    }
} apply;

template<typename Tuple, typename Applicable>
auto operator|(Tuple&& tuple, Apply<Applicable> const& f)
{
    return std::forward<Tuple>(tuple) | then(f);
}

template<typename Tuple, typename Applicable>
auto operator|(Tuple&& tuple, Apply<Applicable>&& f)
{
    return std::forward<Tuple>(tuple) | then(std::move(f));
}

enum class in : bool { sequence = false, parallel = true };
template<typename T>
struct When
{
    in manner;
    T value;
};
template<typename T>
When<std::decay_t<T>> operator^(in manner, T&& value)
{
    return {manner, std::forward<T>(value)};
}

template<typename T, typename... Args>
auto operator|(When<T>&& when, Get<Args...>&& g)
{
    return std::move(when.value) | std::move(g);
}

template<typename T, typename... Args>
auto operator|(When<T> const& when, Get<Args...>&& g)
{
    return when.value | std::move(g);
}

template<typename T, typename... Args>
auto operator|(When<T>&& when, Get<Args...> const& g)
{
    return std::move(when.value) | g;
}

template<typename T, typename... Args>
auto operator|(When<T> const& when, Get<Args...> const& g)
{
    return when.value | g;
}

template<typename T, typename Invocable>
auto operator|(When<T>&& when, Then<Invocable>&& f)
{
    return std::move(when.value) | std::move(f);
}

template<typename T, typename Invocable>
auto operator|(When<T> const& when, Then<Invocable>&& f)
{
    return when.value | std::move(f);
}

template<typename T, typename Invocable>
auto operator|(When<T>&& when, Then<Invocable> const& f)
{
    return std::move(when.value) | f;

}

template<typename T, typename Invocable>
auto operator|(When<T> const& when, Then<Invocable> const& f)
{
    return when.value | f;
}

template<typename Monad>
auto operator&(in manner, Monad&& monad)
{
    return manner ^ (std::forward<Monad>(monad) | then(make_tuple));
}

static inline constexpr struct {
    template<typename Head, typename... Tail>
    auto operator()(in manner, Head&& head, Tail&& ... tail) const
    {
        return ((manner ^ std::forward<Head>(head)) ^ ... ^ std::forward<Tail>(tail)).value;
    }

    template<typename Head, typename... Tail>
    auto operator()(Head&& head, Tail&& ... tail) const
    {
        return (*this)(in::sequence, std::forward<Head>(head), std::forward<Tail>(tail)...);
    }
} when_any;

static constexpr inline struct {
    template<typename Head, typename... Tail>
    auto operator()(in manner, Head&& head, Tail&& ... tail) const
    {
        return ((manner & std::forward<Head>(head)) & ... & std::forward<Tail>(tail)).value;
    }

    template<typename Head, typename... Tail>
    auto operator()(Head&& head, Tail&& ... tail) const
    {
        return (*this)(in::sequence, std::forward<Head>(head), std::forward<Tail>(tail)...);
    }
} when_all;

};
