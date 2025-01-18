#include <string>

#include "Errors.hpp"
#include "Repr.hpp"

#include <sstream>

using std::string;

namespace Errors {
	const string WrongArgs = "Server started with invalid number of arguments";

	namespace Config {
		const string OpeningError = "Failed to open configuration file";
	}
}

const string Errors::Config::ParseError(Tokens& tokens) {
	if (tokens.empty())
		return "Failed to parse configuration file: Unexpected end of file";
	return "Failed to parse configuration file: Unexpected token: '" + tokens.front().second + "'";
}

const string Errors::Config::DirectiveArgumentEmpty(const string& directive) {
	return "Argument for directive '" + directive + "' must not be empty";
}

const string Errors::Config::DirectiveArgumentInvalidPort(const string& directive, const string& argument) {
	return "Port argument '" + argument + "' for directive '" + directive + "' is invalid";
}

const string Errors::Config::DirectiveArgumentNotNumeric(const string& directive, const string& argument) {
	return "Number argument '" + argument + "' for directive '" + directive + "' is not numeric";
}

const string Errors::Config::DirectiveArgumentNotUnique(const string& directive, const string& argument) {
	return "Argument '" + argument + "' for directive '" + directive + "' is not unique";
}

const string Errors::Config::DirectiveArgumentPortNumberTooHigh(const string& directive, const string& argument) {
	return "Port argument '" + argument + "' for directive '" + directive + "' is too high";
}

const string Errors::Config::DirectiveArgumentPortNumberTooLow(const string& directive, const string& argument) {
	return "Port argument '" + argument + "' for directive '" + directive + "' is too low";
}

const string Errors::Config::DirectiveIPv6NotSupported(const string& directive) {
	return "IP address argument for directive '" + directive + "' must not be IPv6";
}

const string Errors::Config::DirectiveInvalidBooleanArgument(const string& directive, const string& argument) {
	return "Bool argument '" + argument + "' for directive '" + directive + "' is invalid";
}

const string Errors::Config::DirectiveInvalidIpAddressArgument(const string& directive, const string& argument) {
	return "IP address argument '" + argument + "' for directive '" + directive + "' is invalid";
}

const string Errors::Config::DirectiveInvalidSizeArgument(const string& directive, const string& argument) {
	return "Size argument '" + argument + "' for directive '" + directive + "' is invalid";
}

const string Errors::Config::DirectiveInvalidStatusCodeArgument(const string& directive, const string& argument) {
	return "Status code argument '" + argument + "' for directive '" + directive + "' is invalid";
}

const string Errors::Config::InvalidDirectiveArgument(const string& directive, const string& argument, const Arguments& options) {
	return "Argument '" + argument + "' for directive '" + directive + "' is invalid. Must be one of " + ::repr(options);
}

const string Errors::Config::InvalidDirectiveArgumentCount(const string& directive, unsigned int count, unsigned int min, unsigned int max) {
	return "Argument count of " + ::repr(count) + " for directive '" + directive + "' is invalid. Must be at least " + ::repr(min) + " and at most " + ::repr(max);
}

const string Errors::Config::UnknownDirective(const string& directive) {
	return "Unknown directive: '" + directive + "'";
}
