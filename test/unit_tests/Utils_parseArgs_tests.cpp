#include <catch2/catch_all.hpp>

#include <stdexcept>

#include "Constants.hpp"
#include "Utils.hpp"

#define ARGV_NULL_MSG "Argument vector is NULL"
#define ARGV_CORRUPT_MSG "Argument vector is corrupted"
#define ARGC_WRONG_MSG "Server started with invalid number of arguments"

using namespace Catch::Matchers;

// positive tests
TEST_CASE("0 arguments", "[utils][argparse]") {
    int argc = 1;
    const char *argv_[] = {"./webserv", NULL};
    char **argv = const_cast<char **>(argv_);

    CHECK(Utils::parseArgs(argc, argv) == Constants::defaultConfPath);
}

TEST_CASE("0 arguments and corrupted argv", "[utils][argparse]") {
    int argc = 1;
    const char **argv_ = NULL;
    char **argv = const_cast<char **>(argv_);

    CHECK(Utils::parseArgs(argc, argv) == Constants::defaultConfPath);
}

TEST_CASE("1 argument", "[utils][argparse]") {
    int argc = 2;
    const char *argv_[] = {"./webserv", "whatever", NULL};
    char **argv = const_cast<char **>(argv_);

    CHECK(Utils::parseArgs(argc, argv) == "whatever");
}

TEST_CASE("1 empty argument", "[utils][argparse]") {
    int argc = 2;
    const char *argv_[] = {"./webserv", "", NULL};
    char **argv = const_cast<char **>(argv_);

    CHECK(Utils::parseArgs(argc, argv) == "");
}

// negative tests
TEST_CASE("null argv: ac = 2 and argv = NULL", "[utils][argparse]") {
    int argc = 2;
    const char **argv_ = NULL;
    char **argv = const_cast<char **>(argv_);

    CHECK_THROWS_AS(Utils::parseArgs(argc, argv), std::runtime_error);
    CHECK_THROWS_WITH(Utils::parseArgs(argc, argv), ContainsSubstring(ARGV_NULL_MSG));
}

TEST_CASE("corrupted argv: ac = 2 and argv = {NULL}", "[utils][argparse]") {
    int argc = 2;
    const char *argv_[] = {NULL};
    char **argv = const_cast<char **>(argv_);

    CHECK_THROWS_AS(Utils::parseArgs(argc, argv), std::runtime_error);
    CHECK_THROWS_WITH(Utils::parseArgs(argc, argv), ContainsSubstring(ARGV_CORRUPT_MSG));
}

TEST_CASE("corrupted argv: ac = 2 and argv = {\"./webserv\", NULL}", "[utils][argparse]") {
    int argc = 2;
    const char *argv_[] = {"./webserv", NULL};
    char **argv = const_cast<char **>(argv_);

    CHECK_THROWS_AS(Utils::parseArgs(argc, argv), std::runtime_error);
    CHECK_THROWS_WITH(Utils::parseArgs(argc, argv), ContainsSubstring(ARGV_CORRUPT_MSG));
}

TEST_CASE("argc very wrong: ac = 0 and argv = NULL", "[utils][argparse]") {
    int argc = 0;
    const char **argv_ = NULL;
    char **argv = const_cast<char **>(argv_);

    CHECK_THROWS_AS(Utils::parseArgs(argc, argv), std::runtime_error);
    CHECK_THROWS_WITH(Utils::parseArgs(argc, argv), ContainsSubstring(ARGC_WRONG_MSG));
}

TEST_CASE("argc very wrong: ac = 0", "[utils][argparse]") {
    int argc = 0;
    const char *argv_[] = {NULL};
    char **argv = const_cast<char **>(argv_);

    CHECK_THROWS_AS(Utils::parseArgs(argc, argv), std::runtime_error);
    CHECK_THROWS_WITH(Utils::parseArgs(argc, argv), ContainsSubstring(ARGC_WRONG_MSG));
}

TEST_CASE("argc wrong: 2 arguments (ac = 3)", "[utils][argparse]") {
    int argc = 3;
    const char *argv_[] = {"./webserv", "whatever", "another", NULL};
    char **argv = const_cast<char **>(argv_);

    CHECK_THROWS_AS(Utils::parseArgs(argc, argv), std::runtime_error);
    CHECK_THROWS_WITH(Utils::parseArgs(argc, argv), ContainsSubstring(ARGC_WRONG_MSG));
}

TEST_CASE("argc wrong: 3 argument (ac = 4)", "[utils][argparse]") {
    int argc = 4;
    const char *argv_[] = {"./webserv", "whatever", "another", "yet another", NULL};
    char **argv = const_cast<char **>(argv_);

    CHECK_THROWS_AS(Utils::parseArgs(argc, argv), std::runtime_error);
    CHECK_THROWS_WITH(Utils::parseArgs(argc, argv), ContainsSubstring(ARGC_WRONG_MSG));
}
