// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <tuple>
#include <utility>

namespace sib::monad {

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

static inline constexpr struct {
    template<typename T>
    auto operator()(T&& value) const
    {
        return std::make_tuple(std::forward<T>(value));
    }
} make_tuple;

namespace non_tuple{
    using namespace ::sib::monad;

    template<template<typename> class Monad, typename L, typename R>
    Monad<std::tuple<L, R>> operator&(Monad<L> lhs, Monad<R> rhs)
    {
        using ::sib::monad::operator&;
        return (std::move(lhs) | then(make_tuple)) & std::move(rhs);
    }
}

namespace sequence
{
    using namespace ::sib::monad;

    namespace non_tuple {
        using namespace ::sib::monad::non_tuple;

        template<template<typename> class Monad, typename L, typename R>
        Monad<std::tuple<L, R>> operator&(Monad<L> lhs, Monad<R> rhs)
        {
            using ::sib::monad::sequence::operator&;
            return (std::move(lhs) | then(make_tuple)) & std::move(rhs);
        }

        template<template<typename> class Monad, typename L, typename R, typename... LArgs, typename... RArgs>
        Monad<std::tuple<L, R>(LArgs..., RArgs...)> operator&(Monad<L(LArgs...)> lhs, Monad<R(RArgs...)> rhs)
        {
            using ::sib::monad::sequence::operator&;
            return (std::move(lhs) | then(make_tuple)) & std::move(rhs);
        }
    }
}
namespace parallel
{
    using namespace ::sib::monad;
    namespace non_tuple {
        using namespace ::sib::monad::non_tuple;

        template<template<typename> class Monad, typename L, typename R>
        Monad<std::tuple<L, R>> operator&(Monad<L> lhs, Monad<R> rhs)
        {
            using ::sib::monad::parallel::operator&;
            return (std::move(lhs) | then(make_tuple)) & std::move(rhs);
        }

        template<template<typename> class Monad, typename L, typename R, typename... LArgs, typename... RArgs>
        Monad<std::tuple<L, R>(LArgs..., RArgs...)> operator&(Monad<L(LArgs...)> lhs, Monad<R(RArgs...)> rhs)
        {
            using ::sib::monad::parallel::operator&;
            return (std::move(lhs) | then(make_tuple)) & std::move(rhs);
        }
    }
}

enum class in : bool { sequence = false, parallel = true };

static inline constexpr struct {
    template<typename Head, typename... Tail>
    auto operator()(in manner, Head&& head, Tail&& ... tail) const
    {
        if (manner == in::sequence) {
            using namespace ::sib::monad::sequence;
            return (std::forward<Head>(head) ^ ... ^ std::forward<Tail>(tail));
        } else {
            using namespace ::sib::monad::parallel;
            return (std::forward<Head>(head) ^ ... ^ std::forward<Tail>(tail));
        }
    }

    template<typename Head, typename... Tail>
    auto operator()(Head&& head, Tail&& ... tail) const
    {
        return (*this)(in::sequence, std::forward<Head>(head), std::forward<Tail>(tail)...);
    }
} when_any;

template<typename Invokable>
class Then
{
private:
    Invokable invokable;

public:
    explicit Then(Invokable callable)
        : invokable{std::move(callable)}
    {}

    template<typename... Args>
    auto operator()(Args&&... args) const &
    {
        return std::invoke(invokable, std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto operator()(Args&&... args) &&
    {
        return std::invoke(std::move(invokable), std::forward<Args>(args)...);
    }
};
static constexpr inline struct {
    template<typename Invokable>
    Then<std::decay_t<Invokable>> operator()(Invokable&& invokable) const
    {
        return Then<std::decay_t<Invokable>>{std::forward<Invokable>(invokable)};
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

static constexpr inline struct {
    template<typename Head, typename... Tail>
    auto operator()(in manner, Head&& head, Tail&& ... tail) const
    {
        if (manner == in::sequence) {
            using namespace sequence;
            return ((std::forward<Head>(head) | then(make_tuple)) & ... & std::forward<Tail>(tail));
        } else {
            using namespace parallel;
            return ((std::forward<Head>(head) | then(make_tuple)) & ... & std::forward<Tail>(tail));
        }
    }

    template<typename Head, typename... Tail>
    auto operator()(Head&& head, Tail&& ... tail) const
    {
        return (*this)(in::sequence, std::forward<Head>(head), std::forward<Tail>(tail)...);
    }
} when_all;

template<typename Tuple, typename Applicable>
auto operator|(Tuple&& tuple, Apply<Applicable> const& apply)
{
    return std::forward<Tuple>(tuple) | then(apply);
}

template<typename Tuple, typename Applicable>
auto operator|(Tuple&& tuple, Apply<Applicable>&& apply)
{
    return std::forward<Tuple>(tuple) | then(std::move(apply));
}

};
