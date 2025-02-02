// #define CATCH_CONFIG_MAIN
// #include <catch2/catch_all.hpp> // bloat

#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("simple test", "[utils]") { REQUIRE(1 + 1 == 2); }
