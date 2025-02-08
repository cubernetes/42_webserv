#pragma once /* Utils.hpp */

#include <cstddef>
#include <ctime>
#include <ios>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <vector>

#include "MacroMagic.h"

using std::string;
using std::vector;

struct sockaddr_in_wrapper {
    struct in_addr sin_addr;
    in_port_t sin_port;
    sockaddr_in_wrapper(sockaddr_in saddr) : sin_addr(saddr.sin_addr), sin_port(saddr.sin_port) {}
    sockaddr_in_wrapper(struct in_addr _sin_addr, in_port_t _sin_port) : sin_addr(_sin_addr), sin_port(_sin_port) {}
    sockaddr_in_wrapper() : sin_addr(), sin_port() {}
};

struct pollevents_helper {
    short events;
    pollevents_helper(short e) : events(e) {}
    pollevents_helper() : events() {}
};

struct in_port_t_helper {
    in_port_t port;
    in_port_t_helper(in_port_t p) : port(p) {}
    in_port_t_helper() : port() {}
};

namespace Utils {
    string parseArgs(int ac, char **av);
    bool isPrefix(string prefix, string longerString);
    string strToLower(const string &str);
    string strToUpper(const string &str);
    char decodeTwoHexChars(const char _c1, const char _c2);
    bool isHexDigitNoCase(const char c);
    size_t convertSizeToBytes(const string &sizeStr);
    string replaceAll(string s, const string &search, const string &replace);
    string jsonEscape(const string &s);
    string escapeExceptNlAndTab(const string &s);
    string escape(const string &s);
    bool allUppercase(const string &str);
    string ellipsisize(const string &str, size_t maxLen);
    string formattedTimestamp(std::time_t _t = 0, bool forLogger = false);
    string millisecondRemainderSinceEpoch();
    string formatSI(size_t size);

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

// Helper macro to create inplace vectors (i.e. passing as an argument), very useful
// sometimes
#define VEC(type, ...) (Utils::getTmpVec<type>().clear(), FOR_EACH_ONE(TMP_VEC_PUSH_BACK, type, __VA_ARGS__) Utils::getTmpVec<type>())
