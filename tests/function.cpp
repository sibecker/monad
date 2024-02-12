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
    std::function<std::string(std::string const&)> const echo = [](std::string const& s){
        return s;
    };

    SECTION("function | get")
    {
        // get is only supported for non-immediate functions
        CHECK((hello | get) == "Hello"s);
    }

    SECTION("function | flatten")
    {
        std::function<std::function<std::string(std::string const&)>(unsigned int)> const fun_of_fun = [](unsigned int n){
            return [n](std::string const& s){
                std::string result = ""s;
                for(auto i = 0u; i < n; ++i)
                    result += s;
                return result;
            };
        };

        CHECK((hello | flatten | get) == "Hello"s);
        CHECK((fun_of_fun | flatten)("Hello", 2) == "HelloHello"s);
    }
}
