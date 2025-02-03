#include "Logger.hpp"

const string &Logger::fatalPrefix = "[ " + ansi::redBg("FATAL") + " ] ";
const string &Logger::errorPrefix = "[ " + ansi::red("ERROR") + " ] ";
const string &Logger::warnPrefix = "[ " + ansi::yellow("WARN ") + " ] ";
const string &Logger::infoPrefix = "[ " + ansi::white("INFO ") + " ] ";
const string &Logger::debugPrefix = "[ " + ansi::rgbP("DEBUG", 146, 131, 116) + " ] ";
const string &Logger::tracePrefix = "[ " + ansi::rgbP("TRACE", 111, 97, 91) + " ] ";

Logger::Logger(std::ostream &_os, Level _logLevel)
    : os(_os), logLevel(_logLevel), fatal(os, FATAL, logLevel), error(os, ERR, logLevel), warn(os, WARN, logLevel),
      info(os, INFO, logLevel), debug(os, DEBUG, logLevel), trace(os, TRACE, logLevel) {
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
    : os(std::cout), logLevel(INFO), fatal(os, FATAL, logLevel), error(os, ERR, logLevel), warn(os, WARN, logLevel),
      info(os, INFO, logLevel), debug(os, DEBUG, logLevel), trace(os, TRACE, logLevel) {}

Logger::Logger(const Logger &other)
    : os(other.os), logLevel(other.logLevel), fatal(other.fatal), error(other.error), warn(other.warn),
      info(other.info), debug(other.debug), trace(other.trace) {}

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
    }
}

Logger::StreamWrapper &Logger::StreamWrapper::operator()(bool printPrefix) {
    if (printPrefix)
        return *this << prefix;
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
