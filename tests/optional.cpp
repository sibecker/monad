// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <catch2/catch_test_macros.hpp>

#include "monad/optional.h"
#include <functional>

TEST_CASE("Test monadic operations on std::optional")
{
    using sib::monad::operator|;
    using sib::monad::get;
    using sib::monad::flatten;
    using sib::monad::when_any;
    using sib::monad::operator^;
    using sib::monad::then;
    using sib::monad::when_all;
    using sib::monad::operator&;
    using sib::monad::apply;

    std::optional<int> opt = 42;
    std::optional<int> const copt = 27;
    std::optional<int> const empty = std::nullopt;
    auto const f = [](auto const x){ return x + x; };

    SECTION("optional | get")
    {
        // Piping optional through Get preserves the reference type
        static_assert(std::is_same_v<decltype(opt | get), int&>, "optional& -> T&");
        static_assert(std::is_same_v<decltype(std::move(opt) | get), int&&>, "optional&& -> T&&");
        static_assert(std::is_same_v<decltype(copt | get), int const&>, "optional const& -> T const&");
        static_assert(std::is_same_v<decltype(std::move(copt) | get), int const&&>, "optional const&& -> T const&&");

        CHECK((opt | get) == 42);
        CHECK_THROWS_AS(empty | get, std::bad_optional_access);
    }

    SECTION("optional | flatten")
    {
        std::optional<std::optional<int>> opt_of_opt = opt;
        std::optional<std::optional<int>> const copt_of_empty = std::nullopt;

        // Piping optional through flatten always returns optional by value
        static_assert(std::is_same_v<decltype(std::move(opt) | flatten), std::optional<int>>);
        static_assert(std::is_same_v<decltype(copt | flatten), std::optional<int>>);
        static_assert(std::is_same_v<decltype(std::move(opt_of_opt) | flatten), std::optional<int>>);
        static_assert(std::is_same_v<decltype(copt_of_empty | flatten), std::optional<int>>);

        CHECK((opt | flatten) == 42);
        CHECK((opt_of_opt | flatten | get) == 42);
        CHECK((copt_of_empty | flatten) == empty);
    }

    SECTION("when_any(optional...)")
    {
        CHECK(when_any(empty, opt, copt) != empty);
        CHECK(when_any(empty, empty) == empty);

        // operator^ is shorthand for when_any
        CHECK((empty ^ opt) == opt);
        CHECK(((copt ^ empty) | get) == 27);
    }

    SECTION("optional | then")
    {
        // then always returns by value, even if the callable returns a reference
        auto const g = [](int& x) -> int& { return x; };
        int i = 0;
        static_assert(std::is_same_v<decltype(then(g)(i)), int>);

        static_assert(std::is_same_v<decltype(opt | then(f)), std::optional<int>>);
        static_assert(std::is_same_v<decltype(copt | then(f)), std::optional<int>>);
        static_assert(std::is_same_v<decltype(std::move(opt) | then(f)), std::optional<int>>);
        static_assert(std::is_same_v<decltype(std::move(copt) | then(f)), std::optional<int>>);

        CHECK((copt | then(f) | get) == 54);
        CHECK((empty | then(f)) == empty);
    }

    SECTION("when_all(optional, optional) | apply")
    {
        CHECK((when_all(opt, copt) | apply(std::plus<>{}) | get) == 69);

        // operator& is shorthand for when_all
        CHECK(((opt & copt) | apply(std::plus<>{}) | get) == 69);
    }

    SECTION("(((opt ^ empty) | then(f)) & copt) | apply(minus)")
    {
        // A complex expression using all the (public) monadic operations
        CHECK(((((opt ^ empty) | then(f)) & copt) | apply(std::minus<>{}) | get) == 57);
    }

}
