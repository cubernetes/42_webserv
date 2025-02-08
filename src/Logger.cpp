#include <bits/types/struct_timeval.h>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <ostream>
#include <unistd.h>

#include "Ansi.hpp"
#include "Constants.hpp"
#include "Logger.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

// clang-format off
const string &Logger::fatalPrefix = "[ " + ansi::redBg("FATAL") + "  ] ";
const string &Logger::errorPrefix = "[ " + ansi::red("ERROR") + "  ] ";
const string &Logger::warnPrefix = "[ " + ansi::yellow("WARN") + "   ] ";
const string &Logger::infoPrefix = "[ " + ansi::white("INFO") + "   ] ";
const string &Logger::debugPrefix = "[ " + ansi::rgbP("DEBUG", 146, 131, 116) + "  ] ";
const string &Logger::tracePrefix = "[ " + ansi::rgbP("TRACE", 111, 97, 91) + "  ] ";
const string &Logger::trace2Prefix = "[ " + ansi::rgbP("TRACE2", 111, 97, 91) + " ] "; // print full objects on construction/destruction
const string &Logger::trace3Prefix = "[ " + ansi::rgbP("TRACE3", 111, 97, 91) + " ] "; // print object parameters as keyword-arguments
const string &Logger::trace4Prefix = "[ " + ansi::rgbP("TRACE4", 111, 97, 91) + " ] "; // print aggregrate types verbosely with std:: prefix any everything
const string &Logger::trace5Prefix = "[ " + ansi::rgbP("TRACE5", 111, 97, 91) + " ] "; // print as json (no color)
// clang-format on

Logger::Logger(std::ostream &_os, Level _logLevel)
    : os(_os), logLevel(_logLevel), fatal(os, FATAL, logLevel), error(os, ERR, logLevel),
      warn(os, WARN, logLevel), info(os, INFO, logLevel), debug(os, DEBUG, logLevel),
      trace(os, TRACE, logLevel), trace2(os, TRACE2, logLevel),
      trace3(os, TRACE3, logLevel), trace4(os, TRACE4, logLevel),
      trace5(os, TRACE5, logLevel) {
    if (logLevel == DEBUG)
        debug() << "Initialized Logger with logLevel: " << debug.prefix << std::endl;
    else if (logLevel == TRACE)
        debug() << "Initialized Logger with logLevel: " << trace.prefix << std::endl;
    (void)lastInstance(this);
}

Logger Logger::fallbackInstance;

Logger &Logger::lastInstance(Logger *instance) {
    static Logger *last_instance = &fallbackInstance;

    if (instance) {
        last_instance = instance;
    }

    return *last_instance;
}

Logger::Logger()
    : os(std::cout), logLevel(INFO), fatal(os, FATAL, logLevel), error(os, ERR, logLevel),
      warn(os, WARN, logLevel), info(os, INFO, logLevel), debug(os, DEBUG, logLevel),
      trace(os, TRACE, logLevel), trace2(os, TRACE2, logLevel),
      trace3(os, TRACE3, logLevel), trace4(os, TRACE4, logLevel),
      trace5(os, TRACE5, logLevel) {}

Logger::Logger(const Logger &other)
    : os(other.os), logLevel(other.logLevel), fatal(other.fatal), error(other.error),
      warn(other.warn), info(other.info), debug(other.debug), trace(other.trace),
      trace2(other.trace2), trace3(other.trace3), trace4(other.trace4),
      trace5(other.trace5) {}

Logger &Logger::operator=(Logger &other) {
    (void)other;
    // ::swap(*this, other);
    return *this;
}

void Logger::swap(Logger &other) /* noexcept */ {
    (void)other;
    // kinda wrong, but can't swap stream in c++98, so yeah, just to make it compile
}

Logger::StreamWrapper::StreamWrapper(std::ostream &_os, Level _thisLevel,
                                     Level &_logLevel)
    : prefix(), os(_os), thisLevel(_thisLevel), logLevel(_logLevel) {
    switch (thisLevel) {
    case FATAL:
        prefix = fatalPrefix;
        break;
    case ERR:
        prefix = errorPrefix;
        break;
    case WARN:
        prefix = warnPrefix;
        break;
    case INFO:
        prefix = infoPrefix;
        break;
    case DEBUG:
        prefix = debugPrefix;
        break;
    case TRACE:
        prefix = tracePrefix;
        break;
    case TRACE2:
        prefix = trace2Prefix;
        break;
    case TRACE3:
        prefix = trace3Prefix;
        break;
    case TRACE4:
        prefix = trace4Prefix;
        break;
    case TRACE5:
        prefix = trace5Prefix;
        break;
    }
}

#if STRICT_EVAL
static string formattedPid() { return "[" + cmt(" N/A PID ") + "] "; }
#else
static string formattedPid() { return "[" + cmt(" ") + repr(getpid()) + cmt(" ") + "] "; }
#endif

Logger::StreamWrapper &Logger::StreamWrapper::operator()(bool printPrefix) {
    if (printPrefix)
        return *this << prefix << Utils::formattedTimestamp(0, true) << formattedPid();
    else
        return *this;
}

Logger::StreamWrapper &
Logger::StreamWrapper::operator<<(std::ostream &(*manip)(std::ostream &)) {
    if (thisLevel <= logLevel) {
        os << manip;
    }
    return *this;
}

void swap(Logger &a, Logger &b) /* noexcept */ { a.swap(b); }
