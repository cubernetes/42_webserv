#include <string>

#include "Constants.hpp"
#include "Logger.hpp"

using std::string;

namespace Constants {
	const string defaultConfPath = "conf/default.conf";
	const char commentSymbol = '#';
	const int logLevel = Logger::INFO; // doesn't NEED to be const tho
	const bool kwargLogs = true; // doesn't NEED to be const tho
	const bool verboseLogs = false; // doesn't NEED to be const tho
	const bool jsonTrace = false; // doesn't NEED to be const tho
	const int highestPort = 65535;
	const string defaultPort = "8000";
	const string defaultAddress = "*";
}
