#pragma once /* Logger.hpp */

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

#include "Ansi.hpp"

using std::swap;

class Logger;
void swap(Logger &a, Logger &b) /* noexcept */;

class Logger {
    std::ostream &os;

  public:
    enum Level { FATAL, ERR, WARN, INFO, DEBUG, TRACE };
    Level logLevel;

    class StreamWrapper {
      public:
        StreamWrapper(std::ostream &_os, Level _thisLevel, Level &_logLevel)
            : prefix(), os(_os), thisLevel(_thisLevel), logLevel(_logLevel) {
            switch (thisLevel) {
            case FATAL:
                prefix = "[ " + ansi::redBg("FATAL") + " ] ";
                break;
            case ERR:
                prefix = "[ " + ansi::red("ERROR") + " ] ";
                break;
            case WARN:
                prefix = "[ " + ansi::yellow("WARN ") + " ] ";
                break;
            case INFO:
                prefix = "[ " + ansi::white("INFO ") + " ] ";
                break;
            case DEBUG:
                prefix = "[ " + ansi::rgbP("DEBUG", 146, 131, 116) + " ] ";
                break;
            case TRACE:
                prefix = "[ " + ansi::rgbP("TRACE", 111, 97, 91) + " ] ";
                break;
            }
        }

        StreamWrapper &operator()(bool printPrefix = true) {
            if (printPrefix)
                return *this << prefix;
            else
                return *this;
        }

        template <typename T> StreamWrapper &operator<<(const T &value) {
            if (thisLevel <= logLevel) {
                os << value;
            }
            return *this;
        }

        StreamWrapper &operator<<(std::ostream &(*manip)(std::ostream &)) {
            if (thisLevel <= logLevel) {
                os << manip;
            }
            return *this;
        }
        std::string prefix;

      private:
        std::ostream &os;
        Level thisLevel;
        Level &logLevel;
    };

    Logger(std::ostream &_os, Level _logLevel = INFO)
        : os(_os), logLevel(_logLevel), fatal(os, FATAL, logLevel), error(os, ERR, logLevel), warn(os, WARN, logLevel),
          info(os, INFO, logLevel), debug(os, DEBUG, logLevel), trace(os, TRACE, logLevel) {
        if (logLevel == DEBUG)
            debug() << "Initialized Logger with logLevel: " << debug.prefix << std::endl;
        else if (logLevel == TRACE)
            debug() << "Initialized Logger with logLevel: " << trace.prefix << std::endl;
    }
    Logger()
        : os(std::cout), logLevel(INFO), fatal(os, FATAL, logLevel), error(os, ERR, logLevel), warn(os, WARN, logLevel),
          info(os, INFO, logLevel), debug(os, DEBUG, logLevel), trace(os, TRACE, logLevel) {}
    Logger(const Logger &other)
        : os(other.os), logLevel(other.logLevel), fatal(other.fatal), error(other.error), warn(other.warn),
          info(other.info), debug(other.debug), trace(other.trace) {}
    Logger &operator=(Logger &other) {
        (void)other;
        // ::swap(*this, other);
        return *this;
    }
    void swap(Logger &other) /* noexcept */ {
        (void)other;
        // kinda wrong, but can't swap stream in c++98, so yeah, just to make it compile
    }

    StreamWrapper fatal;
    StreamWrapper error;
    StreamWrapper warn;
    StreamWrapper info;
    StreamWrapper debug;
    StreamWrapper trace;

    bool istrace() { return TRACE <= logLevel; }
    bool isdebug() { return DEBUG <= logLevel; }
    bool isinfo() { return INFO <= logLevel; }
    bool iswarn() { return WARN <= logLevel; }
    bool iserror() { return ERR <= logLevel; }
    bool isfatal() { return FATAL <= logLevel; }
};
