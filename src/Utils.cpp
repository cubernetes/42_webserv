#include <algorithm>
#include <bits/types/struct_timeval.h>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <ios>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/time.h>
#include <utility>

#include "Constants.hpp"
#include "Errors.hpp"
#include "Logger.hpp"
#include "Repr.hpp"
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
    std::pair<string::const_iterator, string::const_iterator> matcher = std::mismatch(prefix.begin(), prefix.end(), longerString.begin());
    if (matcher.first == prefix.end())
        return true;
    return false;
}

char Utils::decodeTwoHexChars(const char _c1, const char _c2) {
    const char c1 = static_cast<char>(std::tolower(_c1));
    const char c2 = static_cast<char>(std::tolower(_c2));
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
    if (('0' <= c && c <= '9') || c == 'a' || c == 'A' || c == 'b' || c == 'B' || c == 'c' || c == 'C' || c == 'd' || c == 'D' ||
        c == 'e' || c == 'E' || c == 'f' || c == 'F')
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
    string replaced = s;
    replaced = Utils::replaceAll(s, "\\", "\\\\");
    replaced = Utils::replaceAll(replaced, "\"", "\\\"");
    replaced = Utils::replaceAll(replaced, "\b", "\\b");
    replaced = Utils::replaceAll(replaced, "\t", "\\t");
    replaced = Utils::replaceAll(replaced, "\n", "\\n");
    replaced = Utils::replaceAll(replaced, "\v", "\\v");
    replaced = Utils::replaceAll(replaced, "\f", "\\f");
    replaced = Utils::replaceAll(replaced, "\r", "\\r");
    return "\"" + replaced + "\"";
}

string Utils::escapeExceptNlAndTab(const string &s) {
    string replaced = s;
    replaced = Utils::replaceAll(s, "\\", "\\\\");
    replaced = Utils::replaceAll(replaced, "\"", "\\\"");
    replaced = Utils::replaceAll(replaced, "\b", "\\b");
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

string Utils::ellipsisize(const string &str, size_t maxLen) {
    if (str.length() <= maxLen)
        return str;
    else if (maxLen == 2)
        return "..";
    else if (maxLen == 1)
        return ".";
    else if (maxLen == 0)
        return "";
    size_t prefixLen = (maxLen - 2) / 2;
    size_t suffixLen = (maxLen - 3) / 2;
    return str.substr(0, prefixLen + 1) + "..." + str.substr(str.length() - suffixLen);
}

// std::tolower could lead to UB when the string contains negative chars
string Utils::strToLower(const string &str) {
    string newStr = str;
    std::transform(newStr.begin(), newStr.end(), newStr.begin(), static_cast<int (*)(int)>(std::tolower));
    return newStr;
}

// std::tolower could lead to UB when the string contains negative chars
string Utils::strToUpper(const string &str) {
    string newStr = str;
    std::transform(newStr.begin(), newStr.end(), newStr.begin(), static_cast<int (*)(int)>(std::toupper));
    return newStr;
}

#if STRICT_EVAL
string Utils::millisecondRemainderSinceEpoch() { return "000"; }
#else
string Utils::millisecondRemainderSinceEpoch() {
    int millisec;
    struct timeval tv;

    // TODO: use clock_gettime() instead
    ::gettimeofday(&tv, NULL);

    millisec = static_cast<int>(std::floor(static_cast<double>(tv.tv_usec) / 1000 + .5)); // Round to nearest millisec
    if (millisec >= 1000) {                                                               // Allow for rounding up to nearest second
        millisec -= 1000;
        tv.tv_sec++;
    }

    // ignore tv.tv_sec
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(3) << millisec;
    return oss.str();
}
#endif

string Utils::formattedTimestamp(std::time_t _t, bool forLogger) {
    std::time_t t;
    char date[BUFSIZ];
    char time[BUFSIZ];

    if (_t == 0)
        t = std::time(NULL);
    else
        t = _t;
    if (forLogger) {
        if (std::strftime(date, sizeof(date), " %Y-%m-%d ", std::localtime(&t)) &&
            std::strftime(time, sizeof(time), "%H:%M:%S.", std::localtime(&t)))
            return string("[") + cmt(date) + kwrd(time) + kwrd(millisecondRemainderSinceEpoch() + " ") + "] ";
        return "[" + cmt(" N/A date, N/A time ") + "] ";
    } else {
        if (std::strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", std::localtime(&t)))
            return date;
        return "1970-01-01 00:00:00";
    }
}

string Utils::formatSI(size_t size) {
    std::ostringstream oss;

    if (size < 1024) {
        oss << size << " B";
    } else if (size < 1024 * 1024) {
        oss << std::fixed << std::setprecision(2) << static_cast<double>(size) / (1024.0) << " KiB";
    } else if (size < 1024 * 1024 * 1024) {
        oss << std::fixed << std::setprecision(2) << static_cast<double>(size) / (1024.0 * 1024.0) << " MiB";
    } else {
        oss << std::fixed << std::setprecision(2) << static_cast<double>(size) / (1024.0 * 1024.0 * 1024.0) << " GiB";
    }

    Logger::lastInstance().debug() << "Converting number " << repr(size) << " to readable SI Units " << repr(oss.str()) << std::endl;
    return oss.str();
}
