#include <cmath>
#include <cstdio>
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
const string &Logger::warningPrefix = "[ " + ansi::yellow("WARNING") + "   ] ";
const string &Logger::infoPrefix = "[ " + ansi::white("INFO") + "   ] ";
const string &Logger::debugPrefix = "[ " + ansi::rgbP("DEBUG", 146, 131, 116) + "  ] ";
const string &Logger::tracePrefix = "[ " + ansi::rgbP("TRACE", 111, 97, 91) + "  ] ";
const string &Logger::trace2Prefix = "[ " + ansi::rgbP("TRACE2", 111, 97, 91) + " ] "; // print full objects on construction/destruction
const string &Logger::trace3Prefix = "[ " + ansi::rgbP("TRACE3", 111, 97, 91) + " ] "; // print object parameters as keyword-arguments
const string &Logger::trace4Prefix = "[ " + ansi::rgbP("TRACE4", 111, 97, 91) + " ] "; // print aggregrate types verbosely with std:: prefix any everything
const string &Logger::trace5Prefix = "[ " + ansi::rgbP("TRACE5", 111, 97, 91) + " ] "; // print as json (no color)
// clang-format on

Logger::Logger(std::ostream &_os, Level _logLevel)
    : os(_os), logLevel(_logLevel), fatal(os, FATAL, logLevel), error(os, ERROR, logLevel), warning(os, WARNING, logLevel),
      info(os, INFO, logLevel), debug(os, DEBUG, logLevel), trace(os, TRACE, logLevel), trace2(os, TRACE2, logLevel),
      trace3(os, TRACE3, logLevel), trace4(os, TRACE4, logLevel), trace5(os, TRACE5, logLevel) {
    debug() << "Initialized Logger with logLevel: ";
    switch (logLevel) {
    case FATAL:
        debug << fatal.prefix;
        break;
    case ERROR:
        debug << error.prefix;
        break;
    case WARNING:
        debug << warning.prefix;
        break;
    case INFO:
        debug << info.prefix;
        break;
    case DEBUG:
        debug << debug.prefix;
        break;
    case TRACE:
        debug << trace.prefix;
        break;
    case TRACE2:
        debug << trace2.prefix;
        break;
    case TRACE3:
        debug << trace3.prefix;
        break;
    case TRACE4:
        debug << trace4.prefix;
        break;
    case TRACE5:
        debug << trace5.prefix;
        break;
    }
    debug << std::endl;
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
    : os(std::cout), logLevel(INFO), fatal(os, FATAL, logLevel), error(os, ERROR, logLevel), warning(os, WARNING, logLevel),
      info(os, INFO, logLevel), debug(os, DEBUG, logLevel), trace(os, TRACE, logLevel), trace2(os, TRACE2, logLevel),
      trace3(os, TRACE3, logLevel), trace4(os, TRACE4, logLevel), trace5(os, TRACE5, logLevel) {}

Logger::Logger(const Logger &other)
    : os(other.os), logLevel(other.logLevel), fatal(other.fatal), error(other.error), warning(other.warning), info(other.info),
      debug(other.debug), trace(other.trace), trace2(other.trace2), trace3(other.trace3), trace4(other.trace4), trace5(other.trace5) {}

Logger &Logger::operator=(Logger &other) {
    (void)other;
    // ::swap(*this, other);
    return *this;
}

void Logger::swap(Logger &other) /* noexcept */ {
    (void)other;
    // kinda wrong, but can't swap stream in c++98, so yeah, just to make it compile
}

Logger::StreamWrapper::StreamWrapper(std::ostream &_os, Level _thisLevel, Level &_logLevel)
    : prefix(), os(_os), thisLevel(_thisLevel), logLevel(_logLevel) {
    switch (thisLevel) {
    case FATAL:
        prefix = fatalPrefix;
        break;
    case ERROR:
        prefix = errorPrefix;
        break;
    case WARNING:
        prefix = warningPrefix;
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

Logger::StreamWrapper &Logger::StreamWrapper::operator<<(std::ostream &(*manip)(std::ostream &)) {
    if (thisLevel <= logLevel) {
        os << manip;
    }
    return *this;
}

void swap(Logger &a, Logger &b) /* noexcept */ { a.swap(b); }
