// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "monad/shared_task.h"

namespace sib::monad {

template<typename R, typename... Args, typename... GArgs>
R operator|(std::packaged_task<R(Args...)> task, Get<GArgs...>&& get)
{
    std::apply(task, std::move(get.args));
    return task.get_future().get();
}

template<typename R, typename... Args, typename... GArgs>
R operator|(std::packaged_task<R(Args...)> task, Get<GArgs...> const& get)
{
    std::apply(task, get.args);
    return task.get_future().get();
}

template<typename Signature>
std::packaged_task<Signature> operator|(std::packaged_task<Signature> task, Flatten)
{
    return std::move(task);
}

template<typename R, typename... InnerArgs, typename... OuterArgs>
std::packaged_task<R(InnerArgs..., OuterArgs...)>
    operator|(std::packaged_task<std::packaged_task<R(InnerArgs...)>(OuterArgs...)> task, Flatten)
{
    return std::packaged_task<R(InnerArgs..., OuterArgs...)>{
#ifdef _MSC_VER
        // Capture by shared_ptr to work round bug in MSVC where packaged_task can't construct from a mutable lambda.
        // See https://github.com/microsoft/STL/issues/321
        [ptr = std::make_shared<decltype(task)>(std::move(task))](InnerArgs... innerArgs, OuterArgs... outerArgs) {
            auto& outerTask = *ptr;
#else
        [outerTask = std::move(task)](InnerArgs... innerArgs, OuterArgs... outerArgs) mutable {
#endif
            outerTask(std::move(outerArgs)...);
            std::packaged_task<R(InnerArgs...)> innerTask = outerTask.get_future().get();
            innerTask(std::move(innerArgs)...);
            return innerTask.get_future().get();
        }
    };
}

namespace sequence {
    template<typename R, typename... Args>
    std::packaged_task<R(Args...)> operator^(std::packaged_task<R(Args...)> lhs, std::packaged_task<R(Args...)> rhs) {
        return std::packaged_task<R(Args...)>{
#ifdef _MSC_VER
            // Capture by shared_ptr to work round bug in MSVC where packaged_task can't construct from a mutable lambda.
            // See https://github.com/microsoft/STL/issues/321
            [lptr = std::make_shared<decltype(lhs)>(std::move(lhs)), rptr = std::make_shared<decltype(rhs)>(std::move(rhs))](Args... args) {
                auto& lhs = *lptr;
                auto& rhs = *rptr;
#else
                [lhs = std::move(lhs), rhs = std::move(rhs)](Args const&... args) mutable {
#endif
                try {
                    return std::move(lhs) | get(args...);
                } catch (...) {}
                return std::move(rhs) | get(args...);
            }
        };
    }
}

namespace parallel {
    template<typename R, typename... Args>
    std::packaged_task<R(Args...)> operator^(std::packaged_task<R(Args...)> lhs, std::packaged_task<R(Args...)> rhs) {
        return std::packaged_task<R(Args...)>{
#ifdef _MSC_VER
            // Capture by shared_ptr to work round bug in MSVC where packaged_task can't construct from a mutable lambda.
            // See https://github.com/microsoft/STL/issues/321
            [lptr = std::make_shared<decltype(lhs)>(std::move(lhs)), rptr = std::make_shared<decltype(rhs)>(std::move(rhs))](Args... args) {
                auto& lhs = *lptr;
                auto& rhs = *rptr;
#else
            [lhs = std::move(lhs), rhs = std::move(rhs)](Args const&... args) mutable {
#endif
                std::array<std::future<R>, 2> futures = {lhs.get_future(), rhs.get_future()};
                shared_task<std::size_t(std::size_t) > first{[](auto x) { return x; }};

                std::thread{[lhs = std::move(lhs), first](Args... args) mutable {
                    lhs(std::move(args)...);
                    first(0);
                }, args...}.detach();
                std::thread{[rhs = std::move(rhs), first](Args... args) mutable {
                    rhs(std::move(args)...);
                    first(1);
                }, args...}.detach();

                auto const index = first.get_future().get();
                try {
                    return futures[index].get();
                } catch (...) {}
                return futures[1 - index].get();
            }
        };
    }
}

template<typename R, typename... Args, typename Invokable>
auto operator|(std::packaged_task<R(Args...)> task, Then<Invokable> then) -> std::packaged_task<std::invoke_result_t<Invokable, R>(Args...)>
{
    return std::packaged_task<std::invoke_result_t<Invokable, R>(Args...)> {
#ifdef _MSC_VER
        [task_ptr = std::make_shared<decltype(task)>(std::move(task)), then_ptr = std::make_shared<decltype(then)>(std::move(then))] (Args... args) {
            auto& task = *task_ptr;
            auto& then = *then_ptr;
#else
        [task = std::move(task), then = std::move(then)] mutable (Args... args) {
#endif
            return std::move(then)(std::move(task) | get(std::move(args)...));
        }
    };
}

}
