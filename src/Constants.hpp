#pragma once /* Constants.hpp */

#include <cstddef>
#include <string>

#define CONSTANTS_CHUNK_SIZE 4096

#ifndef STRICT_EVAL
#define STRICT_EVAL true
#endif

#ifndef PP_DEBUG
#define PP_DEBUG false
#endif

using std::string;

namespace Constants {
    extern bool forceNoColor;
    extern const size_t loggingMaxStringLen;
    extern const size_t clientMaxRequestLineSize;
    extern const size_t clientMaxRequestSizeWithoutBody;
    extern const size_t cgiMaxResponseSizeWithoutBody;
    extern const size_t cgiMaxResponseBodySize;
    extern const int cgiTimeout;
    extern const int multiplexTimeout;
    extern const size_t chunkSize;
    extern const char commentSymbol;
    extern const int logLevel;
    extern const bool kwargLogs;
    extern const bool verboseLogs;
    extern const bool jsonTrace;
    extern const int highestPort;
    extern const string &defaultConfPath;
    extern const string &defaultPort;
    extern const string &defaultAddress;
    extern const string &defaultMimeType;
    extern const string &httpVersionString;
    extern const string &webservVersion;
    extern const string &helpText;
    extern const string &defaultClientMaxBodySize;
    extern enum MultPlexType { SELECT, POLL, EPOLL } defaultMultPlexType;
} // namespace Constants
