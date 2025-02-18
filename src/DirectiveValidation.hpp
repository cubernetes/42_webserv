#pragma once /* AllowedDirectives.hpp */

#include <cstddef>

#include "Config.hpp"

namespace DirectiveValidation {
    void checkDirectives(Config &config);
    bool isHttpUri(const string &str);
    bool isDoubleQuoted(const string &str);
    string decodeDoubleQuotedString(const string &str);
    bool evenNumberOfBackslashes(const string &str, size_t endingAt);
} // namespace DirectiveValidation
