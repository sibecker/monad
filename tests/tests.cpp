// Copyright Stewart Becker 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <catch2/catch_test_macros.hpp>

#include "monad/optional.h"

using sib::monad::operator|;
using sib::monad::get;

TEST_CASE("Test optional | get") {
    std::optional<int> opt = 42;
    std::optional<int> const copt = 42;
    std::optional<int> const empty = std::nullopt;

    static_assert(std::is_same_v<decltype(opt | get), int&>, "optional& -> T&");
    static_assert(std::is_same_v<decltype(std::move(opt) | get), int&&>, "optional&& -> T&&");
    static_assert(std::is_same_v<decltype(copt | get), int const&>, "optional const& -> T const&");
    static_assert(std::is_same_v<decltype(std::move(copt) | get), int const&&>, "optional const&& -> T const&&");

    CHECK((opt | get) == 42);
    CHECK_THROWS_AS(empty | get, std::bad_optional_access);
}
