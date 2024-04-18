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
        if constexpr(std::is_copy_constructible_v<std::decay_t<Callable>>) {
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

}
}
