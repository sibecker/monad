// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include "monad/shared_task.h"
#include "monad/monad.h"

namespace sib::monad {

template<typename R, typename... Args, typename... GArgs>
R operator|(std::packaged_task<R(Args...)> task, Get<GArgs...>&& g)
{
    std::apply(task, std::move(g.args));
    return task.get_future().get();
}

template<typename R, typename... Args, typename... GArgs>
R operator|(std::packaged_task<R(Args...)> task, Get<GArgs...> const& g)
{
    std::apply(task, g.args);
    return task.get_future().get();
}

template<typename R, typename... Args, typename... GArgs>
R operator|(shared_task<R(Args...)> const& task, Get<GArgs...>&& g)
{
    std::apply(task, std::move(g.args));
    return task.get_future().get();
}

template<typename R, typename... Args, typename... GArgs>
R operator|(shared_task<R(Args...)> const& task, Get<GArgs...> const& g)
{
    std::apply(task, g.args);
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
            auto& task = *ptr;
#else
        [task = std::move(task)](InnerArgs... innerArgs, OuterArgs... outerArgs) mutable {
#endif
            return std::move(task) | get(std::move(outerArgs)...) | get(std::move(innerArgs)...);
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

template<typename R, typename... Args, typename Invocable>
auto operator|(std::packaged_task<R(Args...)> task, Then<Invocable> f)
{
    using Result = std::invoke_result_t<Invocable, R>;
    return std::packaged_task<Result(Args...)> {
#ifdef _MSC_VER
        // Capture by shared_ptr to work round bug in MSVC where packaged_task can't construct from a mutable lambda.
        // See https://github.com/microsoft/STL/issues/321
        [task_ptr = std::make_shared<decltype(task)>(std::move(task)),
         then_ptr = std::make_shared<decltype(f)>(std::move(f))] (Args... args) {
            auto& task = *task_ptr;
            auto& f = *then_ptr;
#else
        [task = std::move(task), f = std::move(f)] mutable (Args... args) {
#endif
            return std::move(f)(std::move(task) | get(std::move(args)...));
        }
    } | flatten();
}

template<typename R, typename... Args, typename Invocable>
auto operator|(shared_task<R(Args...)> task, Then<Invocable> f)
{
    using Result = std::invoke_result_t<Invocable, R>;
    return std::packaged_task<Result(Args...)> {
            [task = std::move(task), f = std::move(f)] (Args... args) {
                return f(task | get(std::move(args)...));
            }
    } | flatten();
}

template<typename R, typename... Args>
When<std::packaged_task<R(Args...)>> operator^(When<std::packaged_task<R(Args...)>> lhs, std::packaged_task<R(Args...)> rhs) {
    return {lhs.manner,
        std::packaged_task<R(Args...)>{
#ifdef _MSC_VER
            // Capture by shared_ptr to work round bug in MSVC where packaged_task can't construct from a mutable lambda.
            // See https://github.com/microsoft/STL/issues/321
            [manner = lhs.manner,
             lptr = std::make_shared<decltype(lhs.value)>(std::move(lhs.value)),
             rptr = std::make_shared<decltype(rhs)>(std::move(rhs))](Args... args) {
                auto& lhs = *lptr;
                auto& rhs = *rptr;
#else
            [manner = lhs.manner, lhs = std::move(lhs.value), rhs = std::move(rhs)](Args... args) mutable {
#endif
                if (manner == in::sequence) {
                    try {
                        return std::move(lhs) | get(args...);
                    } catch (...) {}
                    return std::move(rhs) | get(args...);
                } else {
                    std::array<std::future<R>, 2> futures = {lhs.get_future(), rhs.get_future()};
                    shared_task<std::size_t(std::size_t)> const first{[](auto x) { return x; }};

                    std::thread{[lhs = std::move(lhs), first = first](Args... args) mutable {
                        lhs(std::move(args)...);
                        first(0);
                    }, args...}.detach();
                    std::thread{[rhs = std::move(rhs), first = first](Args... args) mutable {
                        rhs(std::move(args)...);
                        first(1);
                    }, args...}.detach();

                    auto const index = first.get_future().get();
                    try {
                        return futures[index].get();
                    } catch (...) {}
                    return futures[1 - index].get();
                }
            }
        }
    };
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

template<typename... Ls, typename R, typename... LArgs, typename... RArgs>
When<std::packaged_task<std::tuple<Ls..., R>(LArgs..., RArgs...)>>
    operator&(When<std::packaged_task<std::tuple<Ls...>(LArgs...)>> lhs, std::packaged_task<R(RArgs...)> rhs)
{
    return {lhs.manner,
        std::packaged_task<std::tuple<Ls..., R>(LArgs..., RArgs...)>{
#ifdef _MSC_VER
            // Capture by shared_ptr to work round bug in MSVC where packaged_task can't construct from a mutable lambda.
            // See https://github.com/microsoft/STL/issues/321
            [manner = lhs.manner,
             lptr = std::make_shared<decltype(lhs.value)>(std::move(lhs.value)),
             rptr = std::make_shared<decltype(rhs)>(std::move(rhs))](LArgs... largs, RArgs... rargs) {
                auto& lhs = *lptr;
                auto& rhs = *rptr;
#else
            [manner = lhs.manner, lhs = std::move(lhs.value), rhs = std::move(rhs)](Args... args) mutable {
#endif
                if (manner == in::sequence) {
                    return std::tuple_cat(std::move(lhs) | get(std::move(largs)...),
                                          std::move(rhs) | then(make_tuple) | get(std::move(rargs)...));
                } else {
                    auto rhs_as_tuple = std::move(rhs) | then(make_tuple);
                    auto lfuture = lhs.get_future();
                    auto rfuture = rhs_as_tuple.get_future();
                    auto lvoid = std::async(std::launch::async, std::move(lhs), std::move(largs)...);
                    auto rvoid = std::async(std::launch::async, std::move(rhs_as_tuple), std::move(rargs)...);
                    std::ignore = lvoid, rvoid;
                    return std::tuple_cat(lfuture.get(), rfuture.get());
                }
            }
        }
    };
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
