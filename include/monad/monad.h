// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

namespace sib::monad {

template<typename... Args>
struct Get
{
    std::tuple<Args...> args;
};
static inline constexpr struct {
    template<typename... Args>
    Get<Args...> operator()(Args&& ... args) const {
        return Get<Args...>{std::forward<Args>(args)...};
    }
} get;

class Flatten {};
static inline constexpr struct {
    Flatten operator()() const {
        return Flatten{};
    }
} flatten;

namespace sequence{}
namespace parallel{}

enum class in : bool { sequence = false, parallel = true };

static inline constexpr struct {
    template<typename Head, typename... Tail>
    auto operator()(in manner, Head&& head, Tail&& ... tail) const {
        if (manner == in::sequence) {
            using namespace ::sib::monad::sequence;
            return (std::forward<Head>(head) ^ ... ^ std::forward<Tail>(tail));
        } else {
            using namespace ::sib::monad::sequence;
            return (std::forward<Head>(head) ^ ... ^ std::forward<Tail>(tail));
        }
    }

    template<typename Head, typename... Tail>
    auto operator()(Head&& head, Tail&& ... tail) const {
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
    Then<std::decay_t<Invokable>> operator()(Invokable&& invokable) const {
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
    Apply<std::decay_t<Applicable>> operator()(Applicable&& applicable) const {
        return Apply<std::decay_t<Applicable>>{std::forward<Applicable>(applicable)};
    }
} apply;

static constexpr inline struct {
    template<typename Head, typename... Tail>
    auto operator()(in manner, Head&& head, Tail&& ... tail) const {
        auto make_tuple = [](auto&& head) { return std::make_tuple(std::forward<decltype(head)>(head)); };
        if (manner == in::sequence) {
            using namespace sequence;
            return ((std::forward<Head>(head) | then(make_tuple)) & ... & std::forward<Tail>(tail));
        } else {
            using namespace parallel;
            return ((std::forward<Head>(head) | then(make_tuple)) & ... & std::forward<Tail>(tail));
        }
    }

    template<typename Head, typename... Tail>
    auto operator()(Head&& head, Tail&& ... tail) const {
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

}
