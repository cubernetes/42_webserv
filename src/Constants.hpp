#pragma once /* Constants.hpp */

#include <cstddef>
#include <string>

#define CONSTANTS_CHUNK_SIZE 4096

using std::string;

namespace Constants {
    extern const int cgiTimeout;
    extern const int multiplexTimeout;
    extern const size_t chunkSize;
    extern const string defaultConfPath;
    extern const char commentSymbol;
    extern const int logLevel;
    extern const bool kwargLogs;
    extern const bool verboseLogs;
    extern const bool jsonTrace;
    extern const int highestPort;
    extern const string &defaultPort;
    extern const string &defaultAddress;
    extern const string &defaultMimeType;
    extern const string &httpVersionString;
    extern const string &webservVersion;
    extern const string &helpText;
    extern enum MultPlexType { SELECT, POLL, EPOLL } defaultMultPlexType;
} // namespace Constants
