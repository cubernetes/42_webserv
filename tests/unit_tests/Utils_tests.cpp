// #define CATCH_CONFIG_MAIN
// #include <catch2/catch_all.hpp> // bloat

#include <catch2/catch_test_macros.hpp>
#include "Utils.hpp"

TEST_CASE("simple test", "[utils]") {
	REQUIRE(1 + 1 == 2);
}
