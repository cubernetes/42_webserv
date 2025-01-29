#include <string>

#include "Constants.hpp"
#include "Logger.hpp"

using std::string;

namespace Constants {
	const int multiplexTimeout = 1000;
	const size_t chunkSize = CONSTANTS_CHUNK_SIZE;
	const string defaultConfPath = "conf/default.conf";
	const char commentSymbol = '#';
	const int logLevel = Logger::DEBUG; // doesn't NEED to be const tho
	const bool kwargLogs = true; // doesn't NEED to be const tho
	const bool verboseLogs = false; // doesn't NEED to be const tho
	const bool jsonTrace = false; // doesn't NEED to be const tho
	const int highestPort = 65535;
	const string defaultPort = "8000";
	const string defaultAddress = "*";
	const string defaultMimeType = "application/octet-stream";
	const string httpVersionString = "HTTP/1.1"; // TODO: @timo: can you make them all string& ???
	enum MultPlexType defaultMultPlexType = POLL;
}
