// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <catch2/catch_test_macros.hpp>

#include "monad/shared_task.h"
#include <string>

using namespace std::string_literals;
using namespace sib::monad;

TEST_CASE("Test basic operations on shared_task")
{
    SECTION("Call")
    {
        std::packaged_task<std::string(std::string const&)> hello{
            [](auto const& arg){
                return "Hello, "s + arg;
            }
        };
        // Construct from packaged_task
        sib::shared_task<std::string(std::string const&)> const task{std::move(hello)};

        std::shared_future<std::string> const& future = task.get_future();
        task("World!");
        CHECK(future.get() == "Hello, World!"s);
    }

    SECTION("Call twice")
    {
        // Construct from mutable, move-only lambda
        sib::shared_task<std::string()> const to_string{
            [calls = std::make_unique<int>(0)]() mutable {
                ++(*calls);
                return std::to_string(*calls);
            }
        };

        to_string();
        to_string();
        CHECK(to_string.get_future().get() == "1"s);
    }

    SECTION("packaged_task | share()")
    {
        std::packaged_task<std::string(std::string const&)> task{
            [](auto const& arg){
                return "Hello, "s + arg;
            }
        };
        auto const shared = std::move(task) | share();
        shared("there.");
        CHECK(shared.get_future().get() == "Hello, there."s);
    }
}