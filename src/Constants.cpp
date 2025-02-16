#include <cstddef>
#include <string>

#include "Constants.hpp"

// leave in .cpp file, for faster compilation
#define HELP_TEXT                                                                                                                          \
    "Usage: webserv [options] CONFIG\n"                                                                                                    \
    "\n"                                                                                                                                   \
    "Options:\n"                                                                                                                           \
    "    -c file: Use an alterantive configuration file.\n"                                                                                \
    "    -l LEVEL: Specify loglevel. Supported loglevels are"                                                                              \
    " FATAL/1, ERROR/2, WARNING/3, INFO/4, DEBUG/5, TRACE/6, TRACE2/7 etc. "                                                               \
    "Default is INFO.\n"                                                                                                                   \
    "    -t: Do not run, just test the configuration file and print confirmation to "                                                      \
    "standard errro.\n"                                                                                                                    \
    "    -T: Same as `-t`, but additionally dump the configuration file to standard "                                                      \
    "output.\n"                                                                                                                            \
    "    -v: Print version information.\n"                                                                                                 \
    "    -h: Print help information.\n"

using std::string;

namespace Constants {
    bool forceNoColor = false;
    const size_t loggingMaxStringLen = 50;
    const size_t clientMaxRequestLineSize = 8196;
    const size_t clientMaxRequestSizeWithoutBody = 16384;
    const size_t cgiMaxResponseSizeWithoutBody = 8196;
    const size_t cgiMaxResponseBodySize = 1L * 10 * 1024 * 1024 * 1024;
    const int cgiTimeout = 5;
    const int multiplexTimeout = 1000; // in milliseconds
    const size_t chunkSize = CONSTANTS_CHUNK_SIZE;
    const char commentSymbol = '#';
    const int highestPort = 65535;
    const string &defaultConfPath = "conf/default.conf";
    const string &defaultPort = "8000";
    const string &defaultAddress = "*";
    const string &defaultMimeType = "application/octet-stream";
    const string &httpVersionString = "HTTP/1.1";
    const string &webservVersion = "0.2";
    const string &helpText = HELP_TEXT;
    const string &defaultClientMaxBodySize = "1m"; // k=Kibibyte, m=Mibibyte, g=Gibibyte
    enum MultPlexType defaultMultPlexType = POLL;
} // namespace Constants
