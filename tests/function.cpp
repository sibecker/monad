// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <catch2/catch_test_macros.hpp>

#include "sib/monad/function.h"
#include <string>

TEST_CASE("Test monadic operations on std::function")
{
    using namespace sib::monad;
    using namespace std::string_literals;

    std::function<std::string()> const hello = []{ return "Hello"s; };
    std::function<std::string()> const world = []{ return "World!"s; };
    std::function<std::string(std::string const&)> const echo = [](std::string const& s){
        return s;
    };

    SECTION("function | get()")
    {
        // get is only supported for no-argument functions
        CHECK((hello | get()) == "Hello"s);
    }

    SECTION("function | get(...)")
    {
        // get is only supported for no-argument functions
        CHECK((echo | get("World")) == "World"s);
    }

    SECTION("function | flatten()")
    {
        std::function<std::function<std::string(std::string const&)>(unsigned int)> const fun_of_fun = [](unsigned int n){
            return [n](std::string const& s){
                std::string result = ""s;
                for(auto i = 0u; i < n; ++i)
                    result += s;
                return result;
            };
        };

        CHECK((hello | flatten() | get()) == "Hello"s);
        CHECK((fun_of_fun | flatten())("Hello", 2) == "HelloHello"s);
    }

    SECTION("function | then(...)")
    {
        CHECK((hello | then([](auto const& s){ return s + ", World!"s; }) | get()) == "Hello, World!");
    }

    SECTION("when_any(function...)")
    {
        std::function<std::string()> const hello_or_world = when_any(hello, world);
        CHECK(((hello_or_world() == hello()) || (hello_or_world() == world())));

        std::function<std::string()> const hobsons_choice = in::parallel ^ hello ^ hello ^ hello ^ hello;
        CHECK(hobsons_choice() == "Hello"s);
    }

    SECTION("when_any(function...) with exceptions")
    {
        std::function<std::string()> const except = []() -> std::string { throw std::runtime_error{"Exception!"}; };

        CHECK(((in::sequence ^ hello ^ except) | get()) == "Hello"s);
        CHECK(((in::parallel ^ except ^ hello) | get()) == "Hello"s);
        CHECK_THROWS_AS(((in::sequence ^ except ^ except) | get()), std::runtime_error);
    }

    SECTION("function & function")
    {
        auto const merge = [](std::string const& x, std::string const& y, std::string const& z) {
            return x + ", "s + y + ", "s + z;
        };
        std::function<std::string()> there = []{ return "there"s; };

        CHECK((when_all(hello, there, world) | apply(merge) | get()) == "Hello, there, World!"s);

        CHECK(((in::parallel & hello & there & world) | apply(merge) | get()) == "Hello, there, World!"s);
    }
}
