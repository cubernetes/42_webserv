#include <string>

#include "Config.hpp"
#include "Constants.hpp"
#include "Errors.hpp"
#include "Repr.hpp"

#define CONFIG_ERROR_PREFIX(ctx) "Config error: In context " + repr(ctx) + ": "

using std::string;

const string Errors::WrongArgs(int ac) { return "Server started with invalid number of arguments: " + repr(ac - 1); }

const string Errors::DegenerateArgv(int ac, char **av) {
    unsigned int realAc = 0;
    while (av && av[realAc])
        ++realAc;
    if (av)
        return "Argument vector is corrupted. Count is " + repr(ac) + " and vector until NULL is " + reprArr(av, realAc);
    return "Argument vector is NULL and supposed count is " + repr(ac);
}

const string Errors::MultimapIndex(const string &key) { return "Directives: Multimap Error: Tried to look up non-existing key " + repr(key); }

const string Errors::Config::OpeningError(const string &path) { return "Failed to open configuration file: " + repr(path); }

const string Errors::Config::ParseError(Tokens &tokens) {
    if (tokens.empty())
        return "Failed to parse configuration file: Unexpected end of file";
    return "Failed to parse configuration file: Unexpected token: '" + punct(tokens.front().second) + "'";
}

const string Errors::Config::DirectiveArgumentEmpty(const string &ctx, const string &directive) {
    return CONFIG_ERROR_PREFIX(ctx) + "Argument for directive " + repr(directive) + " must not be empty";
}

const string Errors::Config::DirectiveArgumentInvalidPort(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "Port argument " + repr(argument) + " for directive " + repr(directive) + " is invalid";
}

const string Errors::Config::DirectiveArgumentNotNumeric(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "Number argument " + repr(argument) + " for directive " + repr(directive) + " is not numeric";
}

const string Errors::Config::DirectiveArgumentNotUnique(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "Argument " + repr(argument) + " for directive " + repr(directive) + " is not unique";
}

// Note: This case actually cannot happen, because Directives is a map so keys are always
// unique already
const string Errors::Config::DirectiveNotUnique(const string &ctx, const string &directive) { return CONFIG_ERROR_PREFIX(ctx) + "Directive " + repr(directive) + " is not unique"; }

const string Errors::Config::DirectiveArgumentPortNumberTooHigh(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "Port argument " + repr(argument) + " for directive " + repr(directive) + " is too high";
}

const string Errors::Config::DirectiveArgumentPortNumberTooLow(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "Port argument " + repr(argument) + " for directive " + repr(directive) + " is too low";
}

const string Errors::Config::DirectiveIPv6NotSupported(const string &ctx, const string &directive) {
    return CONFIG_ERROR_PREFIX(ctx) + "IP address for directive " + repr(directive) + " must not be IPv6";
}

const string Errors::Config::DirectiveInvalidBooleanArgument(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "Bool argument " + repr(argument) + " for directive " + repr(directive) + " is invalid";
}

const string Errors::Config::DirectiveInvalidIpAddressArgument(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "IP address " + repr(argument) + " for directive " + repr(directive) + " is invalid";
}

const string Errors::Config::DirectiveInvalidSizeArgument(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "Size argument " + repr(argument) + " for directive " + repr(directive) + " is invalid";
}

const string Errors::Config::DirectiveInvalidStatusCodeArgument(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "Status code argument " + repr(argument) + " for directive " + repr(directive) + " is invalid";
}

const string Errors::Config::InvalidDirectiveArgument(const string &ctx, const string &directive, const string &argument, const Arguments &options) {
    return CONFIG_ERROR_PREFIX(ctx) + "Argument " + repr(argument) + " for directive " + repr(directive) + " is invalid. Must be one of " + repr(options);
}

const string Errors::Config::InvalidDirectiveArgumentCount(const string &ctx, const string &directive, int count, int min, int max) {
    return CONFIG_ERROR_PREFIX(ctx) + "Argument count of " + repr(count) + " for directive " + repr(directive) + " is invalid." +
           (min == max ? " Must be exactly " + repr(min) : " Must be at least " + repr(min) + (max == -1 ? "" : " and at most " + repr(max)));
}

const string Errors::Config::UnknownDirective(const string &ctx, const string &directive) { return CONFIG_ERROR_PREFIX(ctx) + "Unknown directive: " + repr(directive); }

const string Errors::Config::ZeroServers() { return CONFIG_ERROR_PREFIX(const_cast<char *>("http")) + "There must be at least one " + repr(const_cast<char *>("server")) + " directive"; }

const string Errors::Config::DirectiveArgumentInvalidDoubleQuotedString(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "String argument " + repr(argument) + " for directive " + repr(directive) + " is invalid";
}

const string Errors::Config::DirectiveArgumentInvalidHttpUri(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "HTTP URI argument " + repr(argument) + " for directive " + repr(directive) + " is invalid";
}

const string Errors::Config::DirectiveArgumentNotRootedPath(const string &ctx, const string &directive, const string &argument) {
    return CONFIG_ERROR_PREFIX(ctx) + "Path argument " + repr(argument) + " for directive " + repr(directive) + " is not a rooted path (doesn't start with slash)";
}
