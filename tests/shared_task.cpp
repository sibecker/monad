// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <catch2/catch_test_macros.hpp>

#include "monad/task.h"
#include <string>

using namespace std::string_literals;
using namespace sib;
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

TEST_CASE("Test monadic operations on shared_task")
{
    using namespace sib::monad;
    using namespace std::string_literals;

    sib::shared_task<std::string()> const hello{[] { return "Hello"s; }};
    sib::shared_task<std::string()> const world{[] { return "World!"s; }};
    sib::shared_task<std::string(std::string const&)> const echo{[](std::string const& s) {
        return s;
    }};

    SECTION("task | get()")
    {
        CHECK((hello | get()) == "Hello"s);
    }

    SECTION("task | get(...)")
    {
        CHECK((echo | get("World")) == "World"s);
    }

    SECTION("task | flatten()")
    {
        std::packaged_task<sib::shared_task<std::string(std::string const&)>(unsigned int)> task_of_task{
            [](unsigned int n){
                return sib::shared_task<std::string(std::string const&)>{[n](std::string const& s){
                    std::string result = ""s;
                    for(auto i = 0u; i < n; ++i)
                        result += s;
                    return result;
                }};
            }
        };

        auto flattened_task = std::move(task_of_task) | flatten();
        flattened_task("Hello"s, 2);
        CHECK(flattened_task.get_future().get() == "HelloHello"s);
    }

    SECTION("task | then")
    {
        CHECK((hello | then([](auto const& s){ return s + ", World!"s; }) | get()) == "Hello, World!");
    }

    SECTION("when_any(task...)")
    {
        std::packaged_task<std::string()> hello_or_world = when_any(hello, world);
        auto const result = std::move(hello_or_world) | get();
        CHECK((result == "Hello"s || result == "world!"s));

        auto hobsons_choice = in::parallel ^ hello ^ hello ^ hello ^ hello;
        CHECK((std::move(hobsons_choice) | get()) == "Hello"s);
    }

    SECTION("when_any(task...) with exceptions")
    {
        sib::shared_task<std::string()> except{[]() -> std::string { throw std::runtime_error{"Exception!"}; }};

        CHECK(((in::sequence ^ hello ^ except) | get()) == "Hello"s);
        CHECK(((in::sequence ^ except ^ hello) | get()) == "Hello"s);
        CHECK_THROWS_AS((in::sequence ^ except ^ except) | get(), std::runtime_error);
    }

    SECTION("task & task")
    {
        auto const merge = [](std::string const& x, std::string const& y) {
            return x + ", "s + y;
        };

        CHECK(((in::parallel & hello & world) | apply(merge) | get()) == "Hello, World!"s);

        CHECK((when_all(hello, world) | apply(merge) | get()) == "Hello, World!"s);
    }
}
