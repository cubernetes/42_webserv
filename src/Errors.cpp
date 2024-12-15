#include <string>

#include "Errors.hpp"

using std::string;

namespace Errors {
	const string WrongArgs = "Server started with invalid number of arguments";

	namespace Config {
		const string OpeningError = "Failed to open configuration file";
		const string ParseError = "Failed to parse configuration file";
	}
}
