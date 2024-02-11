// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

namespace sib::monad {

static inline constexpr class Get {} get;
static inline constexpr class Flatten {} flatten;

template<typename Callable>
class Then
{
private:
    Callable callable;

public:
    explicit Then(Callable callable_)
        : callable{std::move(callable_)}
    {}

    template<typename... Args>
    auto operator()(Args&&... args) const &
    {
        return std::invoke(callable, std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto operator()(Args&&... args) &&
    {
        return std::invoke(std::move(callable), std::forward<Args>(args)...);
    }
};

template<typename Callable>
Then<std::decay_t<Callable>> then(Callable&& callable)
{
    return Then<std::decay_t<Callable>>{std::forward<Callable>(callable)};
}

}
