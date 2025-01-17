#pragma once /* Errors.hpp */

#include <string>

using std::string;

namespace Errors {
	extern const string WrongArgs;

	namespace Config {
		extern const string OpeningError;
		extern const string ParseError;
	}
}
