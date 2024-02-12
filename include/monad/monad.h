// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

namespace sib::monad {

static inline constexpr class Get {} get;
static inline constexpr class Flatten {} flatten;

template<typename Head, typename... Tail>
auto when_any(Head&& head, Tail&&... tail)
{
    return (std::forward<Head>(head) ^ ... ^ std::forward<Tail>(tail));
}

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

template<typename Invokable>
Then<std::decay_t<Invokable>> then(Invokable&& invokable)
{
    return Then<std::decay_t<Invokable>>{std::forward<Invokable>(invokable)};
}

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

template<typename Applicable>
Apply<std::decay_t<Applicable>> apply(Applicable&& applicable)
{
    return Apply<std::decay_t<Applicable>>{std::forward<Applicable>(applicable)};
}

template<typename Head, typename... Tail>
auto when_all(Head&& head, Tail&&... tail)
{
    auto make_tuple = [](auto&& head){ return std::make_tuple(std::forward<decltype(head)>(head)); };
    return ((std::forward<Head>(head) | then(make_tuple)) & ... & std::forward<Tail>(tail));
}

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

}
