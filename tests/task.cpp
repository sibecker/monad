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


    SECTION("task | get()")
    {
        std::packaged_task<std::string()> hello{[]{ return "Hello"s; }};
        CHECK((std::move(hello) | get()) == "Hello"s);
    }

    SECTION("task | get(...)")
    {
        std::packaged_task<std::string(std::string const&)> echo{[](std::string const& s){
            return s;
        }};
        CHECK((std::move(echo) | get("World")) == "World"s);
    }

    SECTION("task | flatten()")
    {
        std::packaged_task<std::string()> hello{[]{ return "Hello"s; }};
        CHECK((std::move(hello) | flatten() | get()) == "Hello"s);

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

        auto flattened_task = std::move(task_of_task) | flatten();
        flattened_task("Hello"s, 2);
        CHECK(flattened_task.get_future().get() == "HelloHello"s);
    }

    SECTION("task | then")
    {
        std::packaged_task<std::string()> hello{[] { return "Hello"s; }};

        CHECK((std::move(hello) | then([](auto const& s){ return s + ", World!"s; }) | get()) == "Hello, World!");
    }

    SECTION("when_any(task...)")
    {
        std::packaged_task <std::string()> hello{[] { return "Hello"s; }};
        std::packaged_task <std::string()> world{[] { return "World!"s; }};

        std::packaged_task <std::string()> hello_or_world = std::move(when_any(std::move(hello), std::move(world)));
        auto const result = std::move(hello_or_world) | get();
        CHECK((result == "Hello"s || result == "world!"s));

        std::packaged_task < std::string() > hello1{[] { return "Hello"s; }};
        std::packaged_task < std::string() > hello2{[] { return "Hello"s; }};
        std::packaged_task < std::string() > hello3{[] { return "Hello"s; }};
        std::packaged_task < std::string() > hello4{[] { return "Hello"s; }};

        auto hobsons_choice = in::parallel ^ std::move(hello1) ^ std::move(hello2) ^ std::move(hello3) ^ std::move(hello4);
        CHECK((std::move(hobsons_choice) | get()) == "Hello"s);
    }

    SECTION("when_any(task...) with exceptions")
    {
        std::packaged_task<std::string()> hello1{[]{ return "Hello"s; }};
        std::packaged_task<std::string()> hello2{[]{ return "Hello"s; }};

        std::packaged_task<std::string()> except1{[]() -> std::string { throw std::runtime_error{"Exception!"}; }};
        std::packaged_task<std::string()> except2{[]() -> std::string { throw std::runtime_error{"Exception!"}; }};
        std::packaged_task<std::string()> except3{[]() -> std::string { throw std::runtime_error{"Exception!"}; }};
        std::packaged_task<std::string()> except4{[]() -> std::string { throw std::runtime_error{"Exception!"}; }};

        CHECK(((in::parallel ^ std::move(hello1) ^ std::move(except1)) | get()) == "Hello"s);
        CHECK(((in::parallel ^ std::move(except2) ^ std::move(hello2)) | get()) == "Hello"s);
        CHECK_THROWS_AS((in::parallel ^ std::move(except3) ^ std::move(except4)) | get(), std::runtime_error);
    }

    SECTION("task & task")
    {
        std::packaged_task<std::string()> hello{[] { return "Hello"s; }};
        std::packaged_task<std::string()> there{[] { return "there"s; }};
        std::packaged_task<std::string()> world{[] { return "World!"s; }};

        auto const merge = [](std::string const& x, std::string const& y, std::string const& z) {
            return x + ", "s + y + ", "s + z;
        };

        CHECK(((in::parallel & std::move(hello) & std::move(there) & std::move(world)) | apply(merge) | get()) == "Hello, there, World!"s);

        hello = std::packaged_task<std::string()>{[] { return "Hello"s; }};
        there = std::packaged_task<std::string()>{[] { return "there"s; }};
        world = std::packaged_task<std::string()>{[] { return "World!"s; }};
        CHECK((when_all(std::move(hello), std::move(there), std::move(world)) | apply(merge) | get()) == "Hello, there, World!"s);
    }
}
