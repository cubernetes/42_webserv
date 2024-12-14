#include <string>

#include "Constants.hpp"
#include "Logger.hpp"

using std::string;

namespace Constants {
	const string defaultConfPath = "default.conf";
	const int logLevel = Logger::logLevelInfo; // doesn't NEED to be const tho
}
