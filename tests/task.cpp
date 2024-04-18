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

    SECTION("when_any(task...)")
    {
        std::packaged_task <std::string()> hello{[] { return "Hello"s; }};
        std::packaged_task <std::string()> world{[] { return "World!"s; }};

        std::packaged_task < std::string() > hello_or_world = when_any(std::move(hello), std::move(world));
        auto const result = std::move(hello_or_world) | get();
        CHECK((result == "Hello"s || result == "world!"s));

        std::packaged_task < std::string() > hello1{[] { return "Hello"s; }};
        std::packaged_task < std::string() > hello2{[] { return "Hello"s; }};
        std::packaged_task < std::string() > hello3{[] { return "Hello"s; }};
        std::packaged_task < std::string() > hello4{[] { return "Hello"s; }};

        using sib::monad::parallel::operator^;
        auto hobsons_choice = std::move(hello1) ^ std::move(hello2) ^ std::move(hello3) ^ std::move(hello4);
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

        using sib::monad::parallel::operator^;
        CHECK(((std::move(hello1) ^ std::move(except1)) | get()) == "Hello"s);
        CHECK(((std::move(except2) ^ std::move(hello2)) | get()) == "Hello"s);
        CHECK_THROWS_AS((std::move(except3) ^ std::move(except4)) | get(), std::runtime_error);
    }

    SECTION("task | then")
    {
        std::packaged_task<std::string()> hello{[] { return "Hello"s; }};

        CHECK((std::move(hello) | then([](auto const& s){ return s + ", World!"s; }) | get()) == "Hello, World!");
    }

    SECTION("function & function")
    {
        std::packaged_task<std::string()> hello{[] { return "Hello"s; }};
        std::packaged_task<std::string()> world{[] { return "World!"s; }};

        auto const merge = [](std::string const& x, std::string const& y) {
            return x + ", "s + y;
        };

        using sib::monad::parallel::non_tuple::operator&;
        CHECK(((std::move(hello) & std::move(world)) | then(apply(merge)) | get()) == "Hello, World!"s);

        hello = std::packaged_task<std::string()>{[] { return "Hello"s; }};
        world = std::packaged_task<std::string()>{[] { return "World!"s; }};
        CHECK((when_all(std::move(hello), std::move(world)) | then(apply(merge)) | get()) == "Hello, World!"s);
    }
}
