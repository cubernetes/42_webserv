#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>

#include "Constants.hpp"
#include "Errors.hpp"
#include "Utils.hpp"

using std::string;

string Utils::parseArgs(int ac, char **av) {
  if (ac == 1)
    return Constants::defaultConfPath;
  else if (ac == 2) {
    if (av && av[0] && av[1])
      return av[1];
    throw std::runtime_error(Errors::DegenerateArgv(ac, av));
  }
  throw std::runtime_error(Errors::WrongArgs(ac));
}

// https://stackoverflow.com/a/7913978
bool Utils::isPrefix(string prefix, string longerString) {
  std::pair<string::const_iterator, string::const_iterator> matcher =
      std::mismatch(prefix.begin(), prefix.end(), longerString.begin());
  if (matcher.first == prefix.end())
    return true;
  return false;
}

string Utils::strToLower(const string &str) {
  string newStr = str;
  std::transform(newStr.begin(), newStr.end(), newStr.begin(), static_cast<int (*)(int)>(std::tolower));
  return newStr;
}

char Utils::decodeTwoHexChars(const char _c1, const char _c2) {
  const char c1 = static_cast<char>(::tolower(_c1));
  const char c2 = static_cast<char>(::tolower(_c2));
  char v1, v2;
  if ('0' <= c1 && c1 <= '9')
    v1 = c1 - '0';
  else
    v1 = c1 - 'a' + 10;
  if ('0' <= c2 && c2 <= '9')
    v2 = c2 - '0';
  else
    v2 = c2 - 'a' + 10;
  return static_cast<char>(v1 * 16 + v2);
}

bool Utils::isHexDigitNoCase(const char c) {
  if (('0' <= c && c <= '9') || c == 'a' || c == 'A' || c == 'b' || c == 'B' || c == 'c' || c == 'C' || c == 'd' ||
      c == 'D' || c == 'e' || c == 'E' || c == 'f' || c == 'F')
    return true;
  return false;
}

string Utils::replaceAll(string s, const string &search, const string &replace) {
  size_t pos = 0;
  while ((pos = s.find(search, pos)) != string::npos) {
    s.replace(pos, search.length(), replace);
    pos += replace.length();
  }
  return s;
}

string Utils::jsonEscape(const string &s) {
  return Utils::replaceAll(Utils::replaceAll(s, "\\", "\\\\"), "\"", "\\\"");
}

string Utils::escape(const string &s) {
  string replaced = s;
  replaced = Utils::replaceAll(s, "\\", "\\\\");
  replaced = Utils::replaceAll(replaced, "\"", "\\\"");
  replaced = Utils::replaceAll(replaced, "\a", "\\a");
  replaced = Utils::replaceAll(replaced, "\b", "\\b");
  replaced = Utils::replaceAll(replaced, "\t", "\\t");
  replaced = Utils::replaceAll(replaced, "\n", "\\n");
  replaced = Utils::replaceAll(replaced, "\v", "\\v");
  replaced = Utils::replaceAll(replaced, "\f", "\\f");
  replaced = Utils::replaceAll(replaced, "\r", "\\r");
  return "\"" + replaced + "\"";
}

bool Utils::allUppercase(const string &str) {
  for (size_t i = 0; i < str.length(); ++i) {
    if (str[i] < 'A' || str[i] > 'Z')
      return false;
  }
  return true;
}

size_t Utils::convertSizeToBytes(const string &sizeStr) {
  if (sizeStr.empty())
    return 0;

  size_t size = 0;
  string numStr;
  char unit = sizeStr[sizeStr.length() - 1];

  if (std::isdigit(unit)) {
    numStr = sizeStr;
  } else {
    numStr = sizeStr.substr(0, sizeStr.length() - 1);
    unit = static_cast<char>(std::tolower(unit));
  }

  size = static_cast<size_t>(std::atol(numStr.c_str()));

  switch (unit) {
  case 'k':
    size *= 1024;
    break;
  case 'm':
    size *= 1024 * 1024;
    break;
  case 'g':
    size *= 1024 * 1024 * 1024;
    break;
  }

  return size;
}
