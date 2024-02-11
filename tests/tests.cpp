// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <catch2/catch_test_macros.hpp>

#include "monad/optional.h"

TEST_CASE("Test monadic operations on std::optional")
{
    using sib::monad::operator|;
    using sib::monad::get;
    using sib::monad::flatten;
    using sib::monad::when_any;
    using sib::monad::operator^;

    std::optional<int> opt = 42;
    std::optional<int> const copt = 27;
    std::optional<int> const empty = std::nullopt;

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

        // ^ is shorthand for when_any
        CHECK((empty ^ opt) == opt);
        CHECK(((copt ^ empty) | get) == 27);
    }
}
