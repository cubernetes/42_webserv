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
    enum Level { FATAL, ERROR, WARNING, INFO, DEBUG, TRACE, TRACE2, TRACE3, TRACE4, TRACE5 };
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
    StreamWrapper warning;
    StreamWrapper info;
    StreamWrapper debug;
    StreamWrapper trace;
    StreamWrapper trace2;
    StreamWrapper trace3;
    StreamWrapper trace4;
    StreamWrapper trace5;

    bool isfatal() { return FATAL <= logLevel; }
    bool iserror() { return ERROR <= logLevel; }
    bool iswarn() { return WARNING <= logLevel; }
    bool isinfo() { return INFO <= logLevel; }
    bool isdebug() { return DEBUG <= logLevel; }
    bool istrace() { return TRACE <= logLevel; }
    bool istrace2() { return TRACE2 <= logLevel; }
    bool istrace3() { return TRACE3 <= logLevel; }
    bool istrace4() { return TRACE4 <= logLevel; }
    bool istrace5() { return TRACE5 <= logLevel; }

    static const string &fatalPrefix;
    static const string &errorPrefix;
    static const string &warningPrefix;
    static const string &infoPrefix;
    static const string &debugPrefix;
    static const string &tracePrefix;
    static const string &trace2Prefix;
    static const string &trace3Prefix;
    static const string &trace4Prefix;
    static const string &trace5Prefix;

    static Logger fallbackInstance;
    static Logger &lastInstance(Logger *instance = NULL);
};

void swap(Logger &a, Logger &b) /* noexcept */;
