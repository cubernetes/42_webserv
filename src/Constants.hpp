#pragma once /* Constants.hpp */

#include <string>
#include <cstddef>

#define CONSTANTS_CHUNK_SIZE 4096

using std::string;

namespace Constants {
	extern const int multiplexTimeout;
	extern const size_t chunkSize;
	extern const string defaultConfPath;
	extern const char commentSymbol;
	extern const int logLevel;
	extern const bool kwargLogs;
	extern const bool verboseLogs;
	extern const bool jsonTrace;
	extern const int highestPort;
	extern const string defaultPort;
	extern const string defaultAddress;
	extern const string defaultMimeType;
	extern const string httpVersionString;
	extern enum MultPlexType { SELECT, POLL, EPOLL } defaultMultPlexType;
}
