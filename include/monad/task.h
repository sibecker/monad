// Copyright Stewart 12/02/2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <future>
#include "monad/monad.h"

namespace sib::monad {

template<typename R>
R operator|(std::packaged_task<R()> task, Get)
{
    task();
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

}
