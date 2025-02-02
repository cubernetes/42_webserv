#include <string>

#include "Config.hpp"
#include "Constants.hpp"
#include "Errors.hpp"
#include "Repr.hpp"

#define CONFIG_ERROR_PREFIX(ctx) cmt("Config error: ") + cmt("In context ") + repr(ctx) + cmt(": ")

using std::string;

const string Errors::WrongArgs(int ac) {
  return cmt("Server started with invalid number of arguments: ") + repr(ac - 1);
}

const string Errors::DegenerateArgv(int ac, char **av) {
  unsigned int realAc = 0;
  while (av && av[realAc])
    ++realAc;
  if (av)
    return cmt("Argument vector is corrupted. Count is ") + repr(ac) + cmt(" and vector until NULL is ") +
           reprArr(av, realAc, Constants::jsonTrace);
  return cmt("Argument vector is NULL and supposed count is ") + repr(ac);
}

const string Errors::MultimapIndex(const string &key) {
  return cmt("Directives: Multimap Error: Tried to look up non-existing key ") + repr(key);
}

const string Errors::Config::OpeningError(const string &path) {
  return cmt("Failed to open configuration file: ") + repr(path);
}

const string Errors::Config::ParseError(Tokens &tokens) {
  if (tokens.empty())
    return cmt("Failed to parse configuration file: Unexpected end of file");
  return cmt("Failed to parse configuration file: Unexpected token: '") + punct(tokens.front().second) + cmt("'");
}

const string Errors::Config::DirectiveArgumentEmpty(const string &ctx, const string &directive) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Argument for directive ") + repr(directive) + cmt(" must not be empty");
}

const string Errors::Config::DirectiveArgumentInvalidPort(const string &ctx, const string &directive,
                                                          const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Port argument ") + repr(argument) + cmt(" for directive ") + repr(directive) +
         cmt(" is invalid");
}

const string Errors::Config::DirectiveArgumentNotNumeric(const string &ctx, const string &directive,
                                                         const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Number argument ") + repr(argument) + cmt(" for directive ") +
         repr(directive) + cmt(" is not numeric");
}

const string Errors::Config::DirectiveArgumentNotUnique(const string &ctx, const string &directive,
                                                        const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Argument ") + repr(argument) + cmt(" for directive ") + repr(directive) +
         cmt(" is not unique");
}

// Note: This case actually cannot happen, because Directives is a map so keys are always unique already
const string Errors::Config::DirectiveNotUnique(const string &ctx, const string &directive) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Directive ") + repr(directive) + cmt(" is not unique");
}

const string Errors::Config::DirectiveArgumentPortNumberTooHigh(const string &ctx, const string &directive,
                                                                const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Port argument ") + repr(argument) + cmt(" for directive ") + repr(directive) +
         cmt(" is too high");
}

const string Errors::Config::DirectiveArgumentPortNumberTooLow(const string &ctx, const string &directive,
                                                               const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Port argument ") + repr(argument) + cmt(" for directive ") + repr(directive) +
         cmt(" is too low");
}

const string Errors::Config::DirectiveIPv6NotSupported(const string &ctx, const string &directive) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("IP address for directive ") + repr(directive) + cmt(" must not be IPv6");
}

const string Errors::Config::DirectiveInvalidBooleanArgument(const string &ctx, const string &directive,
                                                             const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Bool argument ") + repr(argument) + cmt(" for directive ") + repr(directive) +
         cmt(" is invalid");
}

const string Errors::Config::DirectiveInvalidIpAddressArgument(const string &ctx, const string &directive,
                                                               const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("IP address ") + repr(argument) + cmt(" for directive ") + repr(directive) +
         cmt(" is invalid");
}

const string Errors::Config::DirectiveInvalidSizeArgument(const string &ctx, const string &directive,
                                                          const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Size argument ") + repr(argument) + cmt(" for directive ") + repr(directive) +
         cmt(" is invalid");
}

const string Errors::Config::DirectiveInvalidStatusCodeArgument(const string &ctx, const string &directive,
                                                                const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Status code argument ") + repr(argument) + cmt(" for directive ") +
         repr(directive) + cmt(" is invalid");
}

const string Errors::Config::InvalidDirectiveArgument(const string &ctx, const string &directive,
                                                      const string &argument, const Arguments &options) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Argument ") + repr(argument) + cmt(" for directive ") + repr(directive) +
         cmt(" is invalid. Must be one of ") + repr(options);
}

const string Errors::Config::InvalidDirectiveArgumentCount(const string &ctx, const string &directive, int count,
                                                           int min, int max) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Argument count of ") + repr(count) + cmt(" for directive ") + repr(directive) +
         cmt(" is invalid.") +
         (min == max ? cmt(" Must be exactly ") + repr(min)
                     : cmt(" Must be at least ") + repr(min) + (max == -1 ? "" : cmt(" and at most ") + repr(max)));
}

const string Errors::Config::UnknownDirective(const string &ctx, const string &directive) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Unknown directive: ") + repr(directive);
}

const string Errors::Config::ZeroServers() {
  return CONFIG_ERROR_PREFIX((char *)"http") + cmt("There must be at least one ") + repr((char *)"server") +
         cmt(" directive");
}

const string Errors::Config::DirectiveArgumentInvalidDoubleQuotedString(const string &ctx, const string &directive,
                                                                        const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("String argument ") + repr(argument) + cmt(" for directive ") +
         repr(directive) + cmt(" is invalid");
}

const string Errors::Config::DirectiveArgumentInvalidHttpUri(const string &ctx, const string &directive,
                                                             const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("HTTP URI argument ") + repr(argument) + cmt(" for directive ") +
         repr(directive) + cmt(" is invalid");
}

const string Errors::Config::DirectiveArgumentNotRootedPath(const string &ctx, const string &directive,
                                                            const string &argument) {
  return CONFIG_ERROR_PREFIX(ctx) + cmt("Path argument ") + repr(argument) + cmt(" for directive ") + repr(directive) +
         cmt(" is not a rooted path (doesn't start with slash)");
}
