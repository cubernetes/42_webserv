#pragma once /* Exceptions.hpp */

#include <stdexcept>

class OnlyCheckConfigException : public std::runtime_error {
  public:
    OnlyCheckConfigException()
        : std::runtime_error("Not constructing HttpServer since option to only check config was enabled") {}
};
