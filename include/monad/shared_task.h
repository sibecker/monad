// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include <atomic>
#include <future>
#include <type_traits>
#include "monad/monad.h"

namespace sib {

template<typename Signature>
class shared_task;

template<typename R, typename... Args>
class shared_task<R(Args...)> {
private:
    struct Impl {
        std::shared_future<R> future;
        mutable std::packaged_task<R(Args...)> task;
        mutable std::atomic_flag flag;

        explicit Impl(std::packaged_task<R(Args...)> task) :
            future{task.get_future()},
            task{std::move(task)},
            flag{}
        {}
    };

    std::shared_ptr<Impl const> impl;

public:
    // We want a class invariant that impl is never null, so we have to customise move operations.
    // Move operations should leave the rhs in a valid state.
    // Here we have move construction being identical to copy, but move assignment is a swap.

    // By the rule of 5, we also explicitly declare the destructor and copy operations, even though they are defaulted.

    ~shared_task() noexcept = default;

    // Move construction is copy construction.
    shared_task(shared_task&& rhs) noexcept :
        impl{rhs.impl}
    {}

    shared_task(shared_task const&) noexcept = default;

    shared_task& operator=(shared_task&& rhs) noexcept
    {
        impl.swap(rhs.impl);
        return *this;
    }

    shared_task& operator=(shared_task const&) noexcept = default;

    // IMPLICIT construction from a packaged_task ...
    /*implicit*/ shared_task(std::packaged_task<R(Args...)> task) :
        impl{std::make_shared<Impl>(std::move(task))}
    {}

    // ... but EXPLICIT from anything else (e.g. a lambda, std::function etc.)
    template<typename Callable>
    explicit shared_task(Callable&& callable) :
        impl{nullptr}
    {
        if constexpr(std::is_copy_constructible_v<std::decay_t<Callable>> &&
                std::is_copy_assignable_v<std::decay_t<Callable>> &&
                std::is_invocable_r_v<R, std::decay_t<Callable> const, Args...>) {
            impl = std::make_shared<Impl>(std::packaged_task<R(Args...)>{std::forward<Callable>(callable)});
        } else {
            impl = std::make_shared<Impl>(std::packaged_task<R(Args...)>{
                [ptr = std::make_shared<std::decay_t<Callable>>(std::move(callable))](Args... args){
                    return std::invoke(*ptr, std::move(args)...);
                }
            });
        }
    }

    void operator()(Args... args) const
    {
        if (impl->flag.test_and_set()) {
            impl->future.wait();
        } else {
            impl->task(std::move(args)...);
        }
    }

    std::shared_future<R> const& get_future() const
    {
        return impl->future;
    }
};

namespace monad {

class Share{};
static inline constexpr struct {
    Share operator()() const {
        return Share{};
    }
} share;

template<typename Signature>
shared_task<Signature> operator|(std::packaged_task<Signature> task, Share)
{
    return shared_task<Signature>{std::move(task)};
}

template<typename R, typename... Args, typename... GArgs>
R operator|(shared_task<R(Args...)> const& task, Get<GArgs...>&& get)
{
    std::apply(task, std::move(get.args));
    return task.get_future().get();
}

template<typename R, typename... Args, typename... GArgs>
R operator|(shared_task<R(Args...)> const& task, Get<GArgs...> const& get)
{
    std::apply(task, get.args);
    return task.get_future().get();
}

template<typename Signature>
shared_task<Signature> operator|(shared_task<Signature> const& task, Flatten)
{
    return task;
}

template<typename R, typename... InnerArgs, typename... OuterArgs>
std::packaged_task<R(InnerArgs..., OuterArgs...)>
    operator|(shared_task<shared_task<R(InnerArgs...)>(OuterArgs...)> const& task, Flatten)
{
    return std::packaged_task<R(InnerArgs..., OuterArgs...)>{
        [task = task](InnerArgs... innerArgs, OuterArgs... outerArgs) {
            return task | get(std::move(outerArgs)...) | get(std::move(innerArgs)...);
        }
    };
}

template<typename R, typename... InnerArgs, typename... OuterArgs>
std::packaged_task<R(InnerArgs..., OuterArgs...)>
    operator|(shared_task<std::packaged_task<R(InnerArgs...)>(OuterArgs...)> task, Flatten)
{
    return std::packaged_task<R(InnerArgs..., OuterArgs...)>{
        [task = std::move(task)](InnerArgs... innerArgs, OuterArgs... outerArgs) {
            return task | get(std::move(outerArgs)...) | get(std::move(innerArgs)...);
        }
    };
}

template<typename R, typename... InnerArgs, typename... OuterArgs>
std::packaged_task<R(InnerArgs..., OuterArgs...)>
    operator|(std::packaged_task<shared_task<R(InnerArgs...)>(OuterArgs...)> task, Flatten)
{
    return std::packaged_task<R(InnerArgs..., OuterArgs...)>{
#ifdef _MSC_VER
// Capture by shared_ptr to work round bug in MSVC where packaged_task can't construct from a mutable lambda.
// See https://github.com/microsoft/STL/issues/321
        [ptr = std::make_shared<decltype(task)>(std::move(task))](InnerArgs... innerArgs, OuterArgs... outerArgs) {
            auto& task = *ptr;
#else
        [task = std::move(task)](InnerArgs... innerArgs, OuterArgs... outerArgs) mutable {
#endif
            return std::move(task) | get(std::move(outerArgs)...) | get(std::move(innerArgs)...);
        }
    };
}

template<typename R, typename... Args, typename Invokable>
auto operator|(shared_task<R(Args...)> task, Then<Invokable> then)
{
    using Result = std::invoke_result_t<Invokable, R>;
    return std::packaged_task<Result(Args...)> {
            [task = std::move(task), then = std::move(then)] (Args... args) {
                return then(task | get(std::move(args)...));
            }
    } | flatten();
}

template<typename R, typename... Args>
When<std::packaged_task<R(Args...)>> operator^(in manner, shared_task<R(Args...)> task)
{
    return manner ^ (std::move(task) | then(identity));
}

template<typename R, typename... Args>
When<std::packaged_task<R(Args...)>> operator^(When<std::packaged_task<R(Args...)>> lhs, shared_task<R(Args...)> rhs)
{
    return std::move(lhs) ^ (std::move(rhs) | then(identity));
}

template<typename R, typename... Args>
When<std::packaged_task<std::tuple<R>(Args...)>> operator&(in manner, shared_task<R(Args...)> task)
{
    return manner & (std::move(task) | then(identity));
}

template<typename... Ls, typename R, typename... LArgs, typename... RArgs>
When<std::packaged_task<std::tuple<Ls..., R>(LArgs..., RArgs...)>>
    operator&(When<std::packaged_task<std::tuple<Ls...>(LArgs...)>> lhs, shared_task<R(RArgs...)> rhs)
{
    return std::move(lhs) & (std::move(rhs) | then(identity));
}

}
}
