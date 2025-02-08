#pragma once /* Errors.hpp */

#include <string>

#include "Config.hpp"

using std::string;

namespace Errors {
    const string WrongArgs(int ac);
    const string DegenerateArgv(int ac, char **av);
    const string MultimapIndex(const string &key);

    namespace Config {
        const string OpeningError(const string &path);
        const string ParseError(Tokens &tokens);
        const string DirectiveArgumentEmpty(const string &ctx, const string &directive);
        const string DirectiveArgumentInvalidPort(const string &ctx, const string &directive, const string &argument);
        const string DirectiveArgumentNotNumeric(const string &ctx, const string &directive, const string &argument);
        const string DirectiveArgumentNotUnique(const string &ctx, const string &directive, const string &argument);
        const string DirectiveNotUnique(const string &ctx, const string &directive);
        const string DirectiveArgumentPortNumberTooHigh(const string &ctx, const string &directive, const string &argument);
        const string DirectiveArgumentPortNumberTooLow(const string &ctx, const string &directive, const string &argument);
        const string DirectiveIPv6NotSupported(const string &ctx, const string &directive);
        const string DirectiveInvalidBooleanArgument(const string &ctx, const string &directive, const string &argument);
        const string DirectiveInvalidIpAddressArgument(const string &ctx, const string &directive, const string &argument);
        const string DirectiveInvalidSizeArgument(const string &ctx, const string &directive, const string &argument);
        const string DirectiveInvalidStatusCodeArgument(const string &ctx, const string &directive, const string &argument);
        const string InvalidDirectiveArgument(const string &ctx, const string &directive, const string &argument, const Arguments &options);
        const string InvalidDirectiveArgumentCount(const string &ctx, const string &directive, int count, int min, int max);
        const string UnknownDirective(const string &ctx, const string &directive);
        const string ZeroServers();
        const string DirectiveArgumentInvalidHttpUri(const string &ctx, const string &directive, const string &argument);
        const string DirectiveArgumentInvalidDoubleQuotedString(const string &ctx, const string &directive, const string &argument);
        const string DirectiveArgumentNotRootedPath(const string &ctx, const string &directive, const string &argument);
    } // namespace Config
} // namespace Errors
