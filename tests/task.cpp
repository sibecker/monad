// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <catch2/catch_test_macros.hpp>

#include "monad/task.h"
#include <string>

TEST_CASE("Test monadic operations on std::packaged_task")
{
    using namespace sib::monad;
    using namespace std::string_literals;

    std::packaged_task<std::string()> world{[]{ return "World!"s; }};
    std::packaged_task<std::string(std::string const&)> echo{[](std::string const& s){
        return s;
    }};

    SECTION("task | get")
    {
        // get is only supported for no-argument tasks
        std::packaged_task<std::string()> hello{[]{ return "Hello"s; }};
        CHECK((std::move(hello) | get) == "Hello"s);
    }

    SECTION("function | flatten")
    {
        std::packaged_task<std::string()> hello{[]{ return "Hello"s; }};
        CHECK((std::move(hello) | flatten | get) == "Hello"s);

        std::packaged_task<std::packaged_task<std::string(std::string const&)>(unsigned int)> task_of_task{
            [](unsigned int n){
                return std::packaged_task<std::string(std::string const&)>{[n](std::string const& s){
                    std::string result = ""s;
                    for(auto i = 0u; i < n; ++i)
                        result += s;
                    return result;
                }};
            }
        };

        auto flattened_task = std::move(task_of_task) | flatten;
        flattened_task("Hello"s, 2);
        CHECK(flattened_task.get_future().get() == "HelloHello"s);
    }

    /*
    SECTION("when_any(function...)")
    {
        std::function<std::string()> const hello_or_world = when_any(hello, world);
        CHECK(((hello_or_world() == hello()) || (hello_or_world() == world())));

        auto const hobsons_choice = hello ^ hello ^ hello ^ hello;
        CHECK(hobsons_choice() == "Hello"s);
    }

    SECTION("function | then")
    {
        CHECK((hello | then([](auto const& s){ return s + ", World!"s; }) | get) == "Hello, World!");
    }

    SECTION("function & function")
    {
        auto const merge = [](std::string const& x, std::string const& y) {
            return x + ", "s + y;
        };

        CHECK(((hello & world) | then(apply(merge)) | get) == "Hello, World!"s);
        CHECK((when_all(hello, world) | then(apply(merge)) | get) == "Hello, World!"s);
    }
     */
}
