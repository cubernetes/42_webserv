#pragma once /* Errors.hpp */

#include <string>

using std::string;

namespace Errors {
	extern const string WrongArgs;

	namespace Config {
		extern const string OpeningError;
		extern const string ParseError;
		extern const string DirectiveArgumentEmpty;
		extern const string DirectiveArgumentInvalidPort;
		extern const string DirectiveArgumentNotNumeric;
		extern const string DirectiveArgumentNotUnique;
		extern const string DirectiveArgumentPortNumberTooHigh;
		extern const string DirectiveArgumentPortNumberTooLow;
		extern const string DirectiveIPv6NotSupported;
		extern const string DirectiveInvalidBooleanArgument;
		extern const string DirectiveInvalidIpAddressArgument;
		extern const string DirectiveInvalidSizeArgument;
		extern const string DirectiveInvalidStatusCodeArgument;
		extern const string InvalidDirectiveArgument;
		extern const string InvalidDirectiveArgumentCount;
		extern const string UnknownDirective;
	}
}
