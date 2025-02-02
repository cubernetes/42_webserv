#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>

#include "Constants.hpp"
#include "Logger.hpp"
#include "Options.hpp"

static Logger::Level convertStringToLogLevel(const char *str) {
    std::string s(str);
    if (s == "FATAL" || s == "1")
        return Logger::FATAL;
    else if (s == "ERR" || s == "ERROR" || s == "2")
        return Logger::ERR;
    else if (s == "WARN" || s == "WARNING" || s == "3")
        return Logger::WARN;
    else if (s == "INFO" || s == "4")
        return Logger::INFO;
    else if (s == "DEBUG" || s == "5")
        return Logger::DEBUG;
    else if (s == "TRACE" || s == "6")
        return Logger::TRACE;
    else
        throw std::runtime_error(
            "Option parsing error: Invalid logLevel argument to `-l' option. Run with -h for more information.");
}

Options::Options(int ac, char **av)
    : printVersion(), onlyCheckConfig(), configPath(Constants::defaultConfPath), logLevel(Logger::INFO) {
    (void)ac;
    bool configPathSpecified = false;
    ++av;
    while (*av) {
        string arg(*av);
        if (arg == "-h")
            printHelp = true;
        else if (arg == "-v")
            printVersion = true;
        else if (arg == "-t")
            onlyCheckConfig = std::max(onlyCheckConfig, static_cast<size_t>(1));
        else if (arg == "-T")
            onlyCheckConfig = std::max(onlyCheckConfig, static_cast<size_t>(2));
        else if (arg == "-l") {
            if (av[1]) {
                logLevel = convertStringToLogLevel(av[1]);
                ++av;
            } else {
                throw std::runtime_error("Option parsing error: Excpected loglevel argument to `-l' option. Run with "
                                         "-h for more information.");
            }
        } else if (arg == "-c") {
            if (av[1]) {
                configPath = std::string(av[1]);
                configPathSpecified = true;
                ++av;
            } else {
                throw std::runtime_error("Option parsing error: Excpected path argument to `-c' option. Run with -h "
                                         "for more information.");
            }
        } else if (arg[0] == '-') {
            throw std::runtime_error("Option parsing error: Unknown option: " + arg);
        } else {
            if (configPathSpecified)
                throw std::runtime_error("Option parsing error: Config path was already specified as: '" + configPath +
                                         "'");
            configPath = string(*av);
            configPathSpecified = true;
        }
        ++av;
    }
}
