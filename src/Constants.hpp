#pragma once /* Constants.hpp */

#include <cstddef>
#include <string>

#define CONSTANTS_CHUNK_SIZE 4096

#define HELP_TEXT                                                                                                      \
    "Usage: webserv [options] CONFIG\n"                                                                                \
    "\n"                                                                                                               \
    "Options:\n"                                                                                                       \
    "    -c file: Use an alterantive configuration file.\n"                                                            \
    "    -l LEVEL: Specify loglevel. Supported loglevels are"                                                          \
    " FATAL/1, ERR/ERROR/2, WARN/WARNING/3, INFO/4, DEBUG/5, TRACE/6. Default is INFO.\n"                              \
    "    -t: Do not run, just test the configuration file.\n"                                                          \
    "    -T: Same as `-t`, but additionally dump the configuration file to standard output.\n"                         \
    "    -v: Print version information.\n"                                                                             \
    "    -h: Print help information.\n"

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
