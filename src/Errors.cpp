#include <string>

#include "Errors.hpp"

using std::string;

namespace Errors {
	const string WrongArgs = "Server started with invalid number of arguments";

	namespace Config {
		const string OpeningError                       = "Failed to open configuration file";
		const string ParseError                         = "Failed to parse configuration file";
		const string DirectiveArgumentEmpty             = "Argument for directive must not be empty";
		const string DirectiveArgumentInvalidPort       = "Port argument for directive is invalid";
		const string DirectiveArgumentNotNumeric        = "Number argument for directive is not numeric";
		const string DirectiveArgumentNotUnique         = "Argument for directive is not unique";
		const string DirectiveArgumentPortNumberTooHigh = "Port argument for directive is too high";
		const string DirectiveArgumentPortNumberTooLow  = "Port argument for directive is too low";
		const string DirectiveIPv6NotSupported          = "IP address argument for directive must not be IPv6";
		const string DirectiveInvalidBooleanArgument    = "Bool argument for directive is invalid";
		const string DirectiveInvalidIpAddressArgument  = "IP address argument for directive is invalid";
		const string DirectiveInvalidSizeArgument       = "Size argument for directive is invalid";
		const string DirectiveInvalidStatusCodeArgument = "Status code argument for directive is invalid";
		const string InvalidDirectiveArgument           = "Argument for directive is invalid";
		const string InvalidDirectiveArgumentCount      = "Argument count for directive is invalid";
		const string UnknownDirective                   = "Unknown directive";
	}
}
