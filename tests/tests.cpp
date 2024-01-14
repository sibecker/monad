#include <catch2/catch_test_macros.hpp>

#include "example.h"

TEST_CASE("Basic test case") {
    CHECK(example() == 42);
}
