#pragma once /* Logger.hpp */

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

#include "Ansi.hpp"

using std::swap;

class Logger {
    std::ostream &os;

  public:
    enum Level { FATAL, ERR, WARN, INFO, DEBUG, TRACE };
    Level logLevel;

    class StreamWrapper {
      public:
        StreamWrapper(std::ostream &_os, Level _thisLevel, Level &_logLevel);
        StreamWrapper &operator()(bool printPrefix = true);

        template <typename T> StreamWrapper &operator<<(const T &value) {
            if (thisLevel <= logLevel) {
                os << value;
            }
            return *this;
        }

        StreamWrapper &operator<<(std::ostream &(*manip)(std::ostream &));
        std::string prefix;

      private:
        std::ostream &os;
        Level thisLevel;
        Level &logLevel;
    };

    Logger(std::ostream &_os, Level _logLevel = INFO);
    Logger();
    Logger(const Logger &other);
    Logger &operator=(Logger &other);
    void swap(Logger &other) /* noexcept */;

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

    static const string &fatalPrefix;
    static const string &errorPrefix;
    static const string &warnPrefix;
    static const string &infoPrefix;
    static const string &debugPrefix;
    static const string &tracePrefix;
};

void swap(Logger &a, Logger &b) /* noexcept */;
