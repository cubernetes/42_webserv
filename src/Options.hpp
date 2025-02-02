#pragma once /* Options.hpp */

#include <cstddef>
#include <string>

#include "Logger.hpp"

class Options {
  public:
    Options(int ac, char **av);

    bool printHelp;
    bool printVersion;
    size_t onlyCheckConfig;
    std::string configPath;
    Logger::Level logLevel;
};
