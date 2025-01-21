#pragma once /* Errors.hpp */

#include <string>

#include "Config.hpp"

using std::string;

namespace Errors {
	const string WrongArgs(int ac);

	namespace Config {
		const string OpeningError(const string& path);
		const string ParseError(Tokens& tokens);
		const string DirectiveArgumentEmpty(const string& directive);
		const string DirectiveArgumentInvalidPort(const string& directive, const string& argument);
		const string DirectiveArgumentNotNumeric(const string& directive, const string& argument);
		const string DirectiveArgumentNotUnique(const string& directive, const string& argument);
		const string DirectiveNotUnique(const string& directive);
		const string DirectiveArgumentPortNumberTooHigh(const string& directive, const string& argument);
		const string DirectiveArgumentPortNumberTooLow(const string& directive, const string& argument);
		const string DirectiveIPv6NotSupported(const string& directive);
		const string DirectiveInvalidBooleanArgument(const string& directive, const string& argument);
		const string DirectiveInvalidIpAddressArgument(const string& directive, const string& argument);
		const string DirectiveInvalidSizeArgument(const string& directive, const string& argument);
		const string DirectiveInvalidStatusCodeArgument(const string& directive, const string& argument);
		const string InvalidDirectiveArgument(const string& directive, const string& argument, const Arguments& options);
		const string InvalidDirectiveArgumentCount(const string& directive, unsigned int count, unsigned int min, unsigned int max);
		const string UnknownDirective(const string& directive);
		const string ZeroServers();
		const string EmptyConfig();
	}
}

