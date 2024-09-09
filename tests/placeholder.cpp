#include <rmx/rmx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Placeholder") {
    REQUIRE( rmx::add(2, 2) == 4 );
}
