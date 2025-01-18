#include <string>

#include "Errors.hpp"
#include "Repr.hpp"

#include <sstream>

using std::string;

const string Errors::WrongArgs(int ac) {
	return cmt("Server started with invalid number of arguments: ") + repr(ac);
}

const string Errors::Config::OpeningError(const string& path) {
	return cmt("Failed to open configuration file: ") + repr(path);
}

const string Errors::Config::ParseError(Tokens& tokens) {
	if (tokens.empty())
		return cmt("Failed to parse configuration file: Unexpected end of file");
	return cmt("Failed to parse configuration file: Unexpected token: '") + punct(tokens.front().second) + cmt("'");
}

const string Errors::Config::DirectiveArgumentEmpty(const string& directive) {
	return cmt("Argument for directive ") + repr(directive) + cmt(" must not be empty");
}

const string Errors::Config::DirectiveArgumentInvalidPort(const string& directive, const string& argument) {
	return cmt("Port argument ") + repr(argument) + cmt(" for directive ") + repr(directive) + cmt(" is invalid");
}

const string Errors::Config::DirectiveArgumentNotNumeric(const string& directive, const string& argument) {
	return cmt("Number argument ") + repr(argument) + cmt(" for directive ") + repr(directive) + cmt(" is not numeric");
}

const string Errors::Config::DirectiveArgumentNotUnique(const string& directive, const string& argument) {
	return cmt("Argument ") + repr(argument) + cmt(" for directive ") + repr(directive) + cmt(" is not unique");
}

const string Errors::Config::DirectiveArgumentPortNumberTooHigh(const string& directive, const string& argument) {
	return cmt("Port argument ") + repr(argument) + cmt(" for directive ") + repr(directive) + cmt(" is too high");
}

const string Errors::Config::DirectiveArgumentPortNumberTooLow(const string& directive, const string& argument) {
	return cmt("Port argument ") + repr(argument) + cmt(" for directive ") + repr(directive) + cmt(" is too low");
}

const string Errors::Config::DirectiveIPv6NotSupported(const string& directive) {
	return cmt("IP address argument for directive ") + repr(directive) + cmt(" must not be IPv6");
}

const string Errors::Config::DirectiveInvalidBooleanArgument(const string& directive, const string& argument) {
	return cmt("Bool argument ") + repr(argument) + cmt(" for directive ") + repr(directive) + cmt(" is invalid");
}

const string Errors::Config::DirectiveInvalidIpAddressArgument(const string& directive, const string& argument) {
	return cmt("IP address argument ") + repr(argument) + cmt(" for directive ") + repr(directive) + cmt(" is invalid");
}

const string Errors::Config::DirectiveInvalidSizeArgument(const string& directive, const string& argument) {
	return cmt("Size argument ") + repr(argument) + cmt(" for directive ") + repr(directive) + cmt(" is invalid");
}

const string Errors::Config::DirectiveInvalidStatusCodeArgument(const string& directive, const string& argument) {
	return cmt("Status code argument ") + repr(argument) + cmt(" for directive ") + repr(directive) + cmt(" is invalid");
}

const string Errors::Config::InvalidDirectiveArgument(const string& directive, const string& argument, const Arguments& options) {
	return cmt("Argument ") + repr(argument) + cmt(" for directive ") + repr(directive) + cmt(" is invalid. Must be one of ") + repr(options);
}

const string Errors::Config::InvalidDirectiveArgumentCount(const string& directive, unsigned int count, unsigned int min, unsigned int max) {
	return cmt("Argument count of ") + repr(count) + cmt(" for directive ") + repr(directive) + cmt(" is invalid. Must be at least ") + repr(min) + cmt(" and at most ") + repr(max);
}

const string Errors::Config::UnknownDirective(const string& directive) {
	return cmt("Unknown directive: ") + repr(directive);
}
