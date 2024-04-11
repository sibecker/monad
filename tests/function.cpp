// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
#include <catch2/catch_test_macros.hpp>

#include "monad/function.h"
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

    SECTION("when_any(function...)")
    {
        std::function<std::string()> const hello_or_world = when_any(hello, world);
        CHECK(((hello_or_world() == hello()) || (hello_or_world() == world())));

        auto const hobsons_choice = hello ^ hello ^ hello ^ hello;
        CHECK(hobsons_choice() == "Hello"s);
    }

    SECTION("when_any(function...) with exceptions")
    {
        std::function<std::string()> const except = []() -> std::string { throw std::runtime_error{"Exception!"}; };

        CHECK((hello ^ except)() == "Hello"s);
        CHECK((except ^ hello)() == "Hello"s);
        CHECK_THROWS_AS((except ^ except)(), std::runtime_error);
    }

    SECTION("function | then(...)")
    {
        CHECK((hello | then([](auto const& s){ return s + ", World!"s; }) | get()) == "Hello, World!");
    }

    SECTION("function & function")
    {
        auto const merge = [](std::string const& x, std::string const& y) {
            return x + ", "s + y;
        };

        CHECK(((hello & world) | then(apply(merge)) | get()) == "Hello, World!"s);
        CHECK((when_all(hello, world) | then(apply(merge)) | get()) == "Hello, World!"s);
    }
}
