#pragma once /* Utils.hpp */

#include <sstream>
#include <string>
#include <vector>

#include "MacroMagic.h"

using std::string;
using std::vector;

namespace Utils {
  string parseArgs(int ac, char **av);
  bool isPrefix(string prefix, string longerString);
  string strToLower(const string &str);
  char decodeTwoHexChars(const char _c1, const char _c2);
  bool isHexDigitNoCase(const char c);
  size_t convertSizeToBytes(const string &sizeStr);
  string replaceAll(string s, const string &search, const string &replace);
  string jsonEscape(string s);
  bool allUppercase(const string &str);

  template <typename T> static inline vector<T> &getTmpVec() {
    static vector<T> _;
    return _;
  }

  // TODO: @timo: improve repr to specify no color
  template <typename T> static inline string STR(T x) {
    std::ostringstream oss;
    oss << std::dec << x;
    return oss.str();
  }
} // namespace Utils

#define TMP_VEC_PUSH_BACK(type, x) Utils::getTmpVec<type>().push_back(x),

// Helper macro to create inplace vectors (i.e. passing as an argument), very useful sometimes
#define VEC(type, ...)                                                                                                 \
  (Utils::getTmpVec<type>().clear(), FOR_EACH_ONE(TMP_VEC_PUSH_BACK, type, __VA_ARGS__) Utils::getTmpVec<type>())
