#include <catch2/catch_all.hpp>

#include <stdexcept>
#include <vector>
#include <string>

#include "Utils.hpp"

#define ARGV_NULL_MSG "Argument vector is NULL"

using namespace Catch::Matchers;

// positive tests
TEST_CASE("empty vec", "[utils][VEC]") {
	CHECK_THAT(VEC(std::string), IsEmpty());
}

TEST_CASE("1 string", "[utils][VEC]") {
	std::vector<std::string> testVec;
	testVec.push_back("teststring");

	CHECK_THAT(VEC(std::string, "teststring"), RangeEquals(testVec));
}

TEST_CASE("2 strings", "[utils][VEC]") {
	std::vector<std::string> testVec;
	testVec.push_back("teststring");
	testVec.push_back("another string");

	CHECK_THAT(VEC(std::string, "teststring", "another string"), RangeEquals(testVec));
}

TEST_CASE("3 strings", "[utils][VEC]") {
	std::vector<std::string> testVec;
	testVec.push_back("teststring");
	testVec.push_back("another string");
	testVec.push_back("and another");

	CHECK_THAT(VEC(std::string, "teststring", "another string", "and another"), RangeEquals(testVec));
}

TEST_CASE("3 empty strings", "[utils][VEC]") {
	std::vector<std::string> testVec;
	testVec.push_back("");
	testVec.push_back("");
	testVec.push_back("");

	CHECK_THAT(VEC(std::string, "", "", ""), RangeEquals(testVec));
}

TEST_CASE("0 ints", "[utils][VEC]") {
	std::vector<int> testVec;

	CHECK_THAT(VEC(int), IsEmpty());
}

TEST_CASE("1 int", "[utils][VEC]") {
	std::vector<int> testVec;
	testVec.push_back(42);

	CHECK_THAT(VEC(int, 42), RangeEquals(testVec));
}

TEST_CASE("0 vecs of strings", "[utils][VEC]") {
	std::vector<std::vector<std::string> > testVec;

	CHECK_THAT(VEC(std::vector<string>), IsEmpty());
}

TEST_CASE("2 vecs of some strings", "[utils][VEC]") {
	std::vector<std::vector<std::string> > testVec;
	std::vector<string> stringVec(2, "stuff");
	std::vector<string> stringVec2(3, "more stuff");
	testVec.push_back(stringVec);
	testVec.push_back(stringVec2);

	CHECK_THAT(VEC(std::vector<std::string>, stringVec, stringVec2), RangeEquals(testVec));
}

TEST_CASE("nested VEC calls with parentheses -> vec<vec<int>>", "[utils][VEC]") {
	std::vector<std::vector<int> > testVec;
	std::vector<int> stringVec(2, 42);
	std::vector<int> stringVec2(3, 21);
	testVec.push_back(stringVec);
	testVec.push_back(stringVec2);

	CHECK_THAT(VEC(std::vector<int>, (VEC(int, 42, 42)), (VEC(int, 21, 21, 21))), RangeEquals(testVec));
}

// negative tests
// no negative tests
