#pragma once /* Repr.hpp */
// This file MUST be included AFTER all of the following includes (to avoid circular dependencies)
// - HttpServer.hpp
// - CgiHandler.hpp

#include <ctime>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <sys/poll.h>
#include <time.h>
#include <utility>
#include <vector>

#include "Ansi.hpp"
#include "Config.hpp"
#include "Constants.hpp"
#include "Reflection.hpp"
#include "Utils.hpp"

#define ANSI_FG "41;41;41"
#define ANSI_STR "184;187;38"
#define ANSI_CHR "211;134;155"
#define ANSI_KWRD "250;189;47"
#define ANSI_PUNCT "254;128;25"
#define ANSI_FUNC "184;187;38"
#define ANSI_NUM "211;134;155"
#define ANSI_VAR "235;219;178"
#define ANSI_CMT "146;131;116"

using ansi::rgb;
using ansi::rgbBg;
using std::map;
using std::multimap;
using std::pair;
using std::set;
using std::string;
using std::vector;

namespace ReprClr {
  string str(string s);
  string chr(string s);
  string kwrd(string s);
  string punct(string s);
  string func(string s);
  string num(string s);
  string var(string s);
  string cmt(string s);
} // namespace ReprClr

using ReprClr::chr;
using ReprClr::cmt;
using ReprClr::func;
using ReprClr::kwrd;
using ReprClr::num;
using ReprClr::punct;
using ReprClr::str;
using ReprClr::var;

void reprInit();
void reprDone();

template <typename T> struct ReprWrapper {
  static inline string repr(const T &value, bool json = false) { return value.repr(json); }
};

template <class T> static inline string getClass(const T &v) {
  (void)v;
  return "(unknown)";
}

// convenience wrapper
template <typename T> static inline string repr(const T &value, bool json = false) {
  return ReprWrapper<T>::repr(value, json);
}

// convenience wrapper for arrays with size
template <typename T> static inline string reprArr(const T *value, size_t size, bool json) {
  std::ostringstream oss;
  if (json)
    oss << "[";
  else if (Constants::verboseLogs)
    oss << punct("{");
  else
    oss << punct("[");
  for (size_t i = 0; i < size; ++i) {
    if (i != 0) {
      if (json)
        oss << ", ";
      else
        oss << punct(", ");
    }
    oss << ReprWrapper<T>::repr(value[i], json);
  }
  if (json)
    oss << "]";
  else if (Constants::verboseLogs)
    oss << punct("}");
  else
    oss << punct("]");
  return oss.str();
}

///// repr partial template specializations

#define INT_REPR(T)                                                                                                    \
  template <> struct ReprWrapper<T> {                                                                                  \
    static inline string repr(const T &value, bool json = false) {                                                     \
      std::ostringstream oss;                                                                                          \
      oss << value;                                                                                                    \
      if (json)                                                                                                        \
        return oss.str();                                                                                              \
      else                                                                                                             \
        return num(oss.str());                                                                                         \
    }                                                                                                                  \
  }

INT_REPR(int);
INT_REPR(short);
INT_REPR(long);
INT_REPR(unsigned int);
INT_REPR(unsigned short);
INT_REPR(unsigned long);
INT_REPR(float);
INT_REPR(double);
INT_REPR(long double);

template <> struct ReprWrapper<bool> {
  static inline string repr(const bool &value, bool json = false) {
    if (json)
      return value ? "true" : "false";
    else
      return num(value ? "true" : "false");
  }
};

template <> struct ReprWrapper<string> {
  static inline string repr(const string &value, bool json = false) {
    if (json)
      return "\"" + Utils::jsonEscape(value) + "\"";
    else
      return str("\"" + Utils::jsonEscape(value) + "\"") + (Constants::verboseLogs ? punct("s") : "");
  }
};

// print generic pointers
template <typename T> struct ReprWrapper<T *> {
  static inline string repr(const T *const &value, bool json = false) {
    std::ostringstream oss;
    if (value)
      oss << value;
    else
      oss << num("NULL");
    if (json)
      return oss.str();
    else
      return num(oss.str());
  }
};

template <> struct ReprWrapper<char *> {
  static inline string repr(const char *const &value, bool json = false) {
    if (json) {
      if (value)
        return "\"" + Utils::jsonEscape(value) + "\"";
      else
        return ReprWrapper<const void *>::repr(value, false);
    } else {
      if (value)
        return str("\"" + Utils::jsonEscape(value) + "\"");
      else
        return ReprWrapper<const void *>::repr(value);
    }
  }
};

// TODO: @timo: escape for char literal
#define CHAR_REPR(T)                                                                                                   \
  template <> struct ReprWrapper<T> {                                                                                  \
    static inline string repr(const T &value, bool json = false) {                                                     \
      if (json)                                                                                                        \
        return string("\"") + Utils::jsonEscape(string(1, (char)value)) + "\"";                                        \
      else                                                                                                             \
        return chr(string("'") + (value == '\\' ? "\\\\" : value == '\'' ? "\\'" : string(1, (char)value)) + "'");     \
    }                                                                                                                  \
  }

CHAR_REPR(char);
CHAR_REPR(unsigned char);
CHAR_REPR(signed char);

#define MAKE_MEMBER_INIT_LIST(_, name) , name()
#define MAKE_DECL(type, name) type name;
#define MAKE_REPR_FN(_, name)                                                                                          \
  string CAT(repr_, name)(bool json) const { return ::repr(name, json); }
#define MAKE_ASSIGN_GETTER(_, name) singleton.name = value.CAT(get, name)();
#define MAKE_ASSIGN_MEMBER(_, name) singleton.name = value.name;
#define MAKE_REFLECT(_, name)                                                                                          \
  members[#name] = std::make_pair((ReprClosure) & ReprWrapper::CAT(repr_, name), &singleton.name);
#define POST_REFLECT_GETTER(clsId, ...)                                                                                \
  static inline string getClass(const clsId &v) {                                                                      \
    (void)v;                                                                                                           \
    return #clsId;                                                                                                     \
  }                                                                                                                    \
  template <> struct ReprWrapper<clsId> : public Reflection {                                                          \
    void reflect() {}                                                                                                  \
    ReprWrapper() : uniqueNameMustComeFirst() FOR_EACH_PAIR(MAKE_MEMBER_INIT_LIST, __VA_ARGS__) {}                     \
    int uniqueNameMustComeFirst;                                                                                       \
    FOR_EACH_PAIR(MAKE_DECL, __VA_ARGS__)                                                                              \
    FOR_EACH_PAIR(MAKE_REPR_FN, __VA_ARGS__)                                                                           \
    static inline string repr(const clsId &value, bool json = false) {                                                 \
      (void)value;                                                                                                     \
      static ReprWrapper<clsId> singleton;                                                                             \
      FOR_EACH_PAIR(MAKE_ASSIGN_GETTER, __VA_ARGS__)                                                                   \
      Members members;                                                                                                 \
      FOR_EACH_PAIR(MAKE_REFLECT, __VA_ARGS__)                                                                         \
      return singleton.reprStruct(#clsId, members, json);                                                              \
    }                                                                                                                  \
  }
#define POST_REFLECT_MEMBER(clsId, ...)                                                                                \
  static inline string getClass(const clsId &v) {                                                                      \
    (void)v;                                                                                                           \
    return #clsId;                                                                                                     \
  }                                                                                                                    \
  template <> struct ReprWrapper<clsId> : public Reflection {                                                          \
    void reflect() {}                                                                                                  \
    ReprWrapper() : uniqueNameMustComeFirst() FOR_EACH_PAIR(MAKE_MEMBER_INIT_LIST, __VA_ARGS__) {}                     \
    ReprWrapper(const ReprWrapper &other)                                                                              \
        : Reflection(other), uniqueNameMustComeFirst() FOR_EACH_PAIR(MAKE_MEMBER_INIT_LIST, __VA_ARGS__) {}            \
    ReprWrapper &operator=(const ReprWrapper &) { return *this; }                                                      \
    int uniqueNameMustComeFirst;                                                                                       \
    FOR_EACH_PAIR(MAKE_DECL, __VA_ARGS__)                                                                              \
    FOR_EACH_PAIR(MAKE_REPR_FN, __VA_ARGS__)                                                                           \
    static inline string repr(const clsId &value, bool json = false) {                                                 \
      (void)value;                                                                                                     \
      static ReprWrapper<clsId> singleton;                                                                             \
      FOR_EACH_PAIR(MAKE_ASSIGN_MEMBER, __VA_ARGS__)                                                                   \
      Members members;                                                                                                 \
      FOR_EACH_PAIR(MAKE_REFLECT, __VA_ARGS__)                                                                         \
      return singleton.reprStruct(#clsId, members, json);                                                              \
    }                                                                                                                  \
  }

POST_REFLECT_MEMBER(struct pollfd, int, fd, short, events, short, revents);

struct in_port_t_helper {
  in_port_t port;
  in_port_t_helper(in_port_t p) : port(p) {}
  in_port_t_helper() : port() {}
};

#include "HttpServer.hpp"
POST_REFLECT_MEMBER(HttpServer::Server, struct in_addr, ip, struct in_port_t_helper, port, vector<string>, serverNames,
                    Directives, directives, LocationCtxs, locations);
POST_REFLECT_MEMBER(HttpServer::CgiProcess, pid_t, pid, int, readFd, int, writeFd, string, response, unsigned long,
                    totalSize, int, clientSocket, const LocationCtx *, location, bool, headersSent, std::time_t,
                    lastActive);
POST_REFLECT_MEMBER(HttpServer::HttpRequest, string, method, string, path, string, httpVersion, HttpServer::Headers,
                    headers, string, body, HttpServer::RequestState, state, size_t, contentLength, bool,
                    chunkedTransfer, size_t, bytesRead, string, temporaryBuffer, bool, pathParsed);
POST_REFLECT_MEMBER(HttpServer::MultPlexFds, MultPlexType, multPlexType, HttpServer::SelectFds, selectFds,
                    HttpServer::PollFds, pollFds, HttpServer::EpollFds, epollFds, HttpServer::FdStates, fdStates);
POST_REFLECT_GETTER(HttpServer, HttpServer::MultPlexFds, _monitorFds, HttpServer::ClientFdToCgiMap, _clientToCgi,
                    HttpServer::CgiFdToClientMap, _cgiToClient, vector<int>, _listeningSockets, HttpServer::PollFds,
                    _pollFds, string, _httpVersionString, HttpServer::PendingWriteMap, _pendingWrites,
                    HttpServer::PendingCloses, _pendingCloses, HttpServer::DefaultServers, _defaultServers,
                    HttpServer::PendingRequests,
                    _pendingRequests); /*, HttpServer::StatusTexts, _statusTexts, HttpServer::MimeTypes, _mimeTypes,
                                          Config, _config, HttpServer::Servers, _servers, string, _rawConfig); */

#include "CgiHandler.hpp"
POST_REFLECT_GETTER(CgiHandler, string, _extension, string, _program);

// for vector
template <typename T> struct ReprWrapper<vector<T> > {
  static inline string repr(const vector<T> &value, bool json = false) {
    std::ostringstream oss;
    if (json)
      oss << "[";
    else if (Constants::verboseLogs)
      oss << kwrd("std") + punct("::") + kwrd("vector") + punct("({");
    else
      oss << punct("[");
    for (size_t i = 0; i < value.size(); ++i) {
      if (i != 0) {
        if (json)
          oss << ", ";
        else
          oss << punct(", ");
      }
      oss << ReprWrapper<T>::repr(value[i], json);
    }
    if (json)
      oss << "]";
    else if (Constants::verboseLogs)
      oss << punct("})");
    else
      oss << punct("]");
    return oss.str();
  }
};

// for map
template <typename K, typename V> struct ReprWrapper<map<K, V> > {
  static inline string repr(const map<K, V> &value, bool json = false) {
    std::ostringstream oss;
    if (json)
      oss << "{";
    else if (Constants::verboseLogs)
      oss << kwrd("std") + punct("::") + kwrd("map") + punct("({");
    else
      oss << punct("{");
    int i = 0;
    for (typename map<K, V>::const_iterator it = value.begin(); it != value.end(); ++it) {
      if (i != 0) {
        if (json)
          oss << ", ";
        else
          oss << punct(", ");
      }
      oss << ReprWrapper<K>::repr(it->first, json);
      if (json)
        oss << ": ";
      else
        oss << punct(": ");
      oss << ReprWrapper<V>::repr(it->second, json);
      ++i;
    }
    if (json)
      oss << "}";
    else if (Constants::verboseLogs)
      oss << punct("})");
    else
      oss << punct("}");
    return oss.str();
  }
};

// for map with comparison function
template <typename K, typename V, typename C> struct ReprWrapper<map<K, V, C> > {
  static inline string repr(const map<K, V, C> &value, bool json = false) {
    std::ostringstream oss;
    if (json)
      oss << "{";
    else if (Constants::verboseLogs)
      oss << kwrd("std") + punct("::") + kwrd("map") + punct("({");
    else
      oss << punct("{");
    int i = 0;
    for (typename map<K, V, C>::const_iterator it = value.begin(); it != value.end(); ++it) {
      if (i != 0) {
        if (json)
          oss << ", ";
        else
          oss << punct(", ");
      }
      oss << ReprWrapper<K>::repr(it->first, json);
      if (json)
        oss << ": ";
      else
        oss << punct(": ");
      oss << ReprWrapper<V>::repr(it->second, json);
      ++i;
    }
    if (json)
      oss << "}";
    else if (Constants::verboseLogs)
      oss << punct("})");
    else
      oss << punct("}");
    return oss.str();
  }
};

// for multimap
template <typename K, typename V> struct ReprWrapper<multimap<K, V> > {
  static inline string repr(const multimap<K, V> &value, bool json = false) {
    std::ostringstream oss;
    if (json)
      oss << "{";
    else if (Constants::verboseLogs)
      oss << kwrd("std") + punct("::") + kwrd("multimap") + punct("({");
    else
      oss << punct("{");
    int i = 0;
    for (typename multimap<K, V>::const_iterator it = value.begin(); it != value.end(); ++it) {
      if (i != 0) {
        if (json)
          oss << ", ";
        else
          oss << punct(", ");
      }
      oss << ReprWrapper<K>::repr(it->first, json);
      if (json)
        oss << ": ";
      else
        oss << punct(": ");
      oss << ReprWrapper<V>::repr(it->second, json);
      ++i;
    }
    if (json)
      oss << "}";
    else if (Constants::verboseLogs)
      oss << punct("})");
    else
      oss << punct("}");
    return oss.str();
  }
};

// for pair
template <typename F, typename S> struct ReprWrapper<pair<F, S> > {
  static inline string repr(const pair<F, S> &value, bool json = false) {
    std::ostringstream oss;
    if (json)
      oss << "[";
    else if (Constants::verboseLogs)
      oss << kwrd("std") + punct("::") + kwrd("pair") + punct("(");
    else
      oss << punct("(");
    oss << ReprWrapper<F>::repr(value.first, json) << (json ? ", " : punct(", "))
        << ReprWrapper<S>::repr(value.second, json);
    if (json)
      oss << "]";
    else
      oss << punct(")");
    return oss.str();
  }
};

// for set
template <typename T> struct ReprWrapper<set<T> > {
  static inline string repr(const set<T> &value, bool json = false) {
    std::ostringstream oss;
    if (json)
      oss << "[";
    else if (Constants::verboseLogs)
      oss << kwrd("std") + punct("::") + kwrd("set") + punct("({");
    else
      oss << punct("{");
    int i = -1;
    for (typename set<T>::const_iterator it = value.begin(); it != value.end(); ++it) {
      if (++i != 0) {
        if (json)
          oss << ", ";
        else
          oss << punct(", ");
      }
      oss << ReprWrapper<T>::repr(*it, json);
    }
    if (json)
      oss << "]";
    else if (Constants::verboseLogs)
      oss << punct("})");
    else
      oss << punct("}");
    return oss.str();
  }
};

// for struct in_addr
template <> struct ReprWrapper<struct in_addr> {
  static inline string repr(const struct in_addr &value, bool json = false) {
    union {
      struct {
        char first;
        char second;
        char third;
        char fourth;
      };
      unsigned int s_addr;
    } addr;
    addr.s_addr = value.s_addr;
    std::ostringstream oss;
    std::ostringstream oss_final;
    oss << (int)addr.first << '.';
    oss << (int)addr.second << '.';
    oss << (int)addr.third << '.';
    oss << (int)addr.fourth;
    oss_final << kwrd("struct in_addr") << punct("(");
    if (json)
      oss_final << oss.str();
    else
      oss_final << num(oss.str());
    oss_final << punct(")");
    return oss_final.str();
  }
};

// for struct in_port_t_helper
template <> struct ReprWrapper<struct in_port_t_helper> {
  static inline string repr(const struct in_port_t_helper &value, bool json = false) {
    std::ostringstream oss;
    oss << ntohs(value.port);
    if (json)
      return oss.str();
    else
      return num(oss.str());
  }
};

#include <sys/epoll.h>
// for struct epoll_event
template <> struct ReprWrapper<struct epoll_event> {
  static inline string repr(const struct epoll_event &value, bool json = false) {
    (void)value;
    std::ostringstream oss;
    oss << "epoll_event(...)";
    if (json)
      return oss.str();
    else
      return kwrd(oss.str());
  }
};

// for enum MultPlexType
template <> struct ReprWrapper<MultPlexType> {
  static inline string repr(const MultPlexType &value, bool json = false) {
    std::ostringstream oss;
    switch (value) {
    case Constants::SELECT:
      oss << "SELECT";
      break;
    case Constants::POLL:
      oss << "POLL";
      break;
    case Constants::EPOLL:
      oss << "EPOLL";
      break;
    default:
      oss << "UNKNOWN";
      break;
    }
    if (json)
      return oss.str();
    else
      return num(oss.str());
  }
};

// for enum RequestState
template <> struct ReprWrapper<HttpServer::RequestState> {
  static inline string repr(const HttpServer::RequestState &value, bool json = false) {
    std::ostringstream oss;
    switch (value) {
    case HttpServer::READING_HEADERS:
      oss << "READING_HEADERS";
      break;
    case HttpServer::READING_BODY:
      oss << "READING_BODY";
      break;
    case HttpServer::REQUEST_COMPLETE:
      oss << "REQUEST_COMPLETE";
      break;
    case HttpServer::REQUEST_ERROR:
      oss << "REQUEST_ERROR";
      break;
    default:
      oss << "UNKNOWN_STATE";
      break;
    }
    if (json)
      return oss.str();
    else
      return num(oss.str());
  }
};

// for enum FdState
template <> struct ReprWrapper<HttpServer::FdState> {
  static inline string repr(const HttpServer::FdState &value, bool json = false) {
    std::ostringstream oss;
    switch (value) {
    case HttpServer::FD_READABLE:
      oss << "READABLE";
      break;
    case HttpServer::FD_WRITEABLE:
      oss << "WRITABLE";
      break;
    case HttpServer::FD_OTHER_STATE:
      oss << "OTHER_STATE";
      break;
    default:
      oss << "UNKNOWN_STATE";
      break;
    }
    if (json)
      return oss.str();
    else
      return num(oss.str());
  }
};

// to print using `std::cout << ...'
template <typename T> static inline std::ostream &operator<<(std::ostream &os, const vector<T> &val) {
  return os << repr(val, Constants::jsonTrace);
}

template <typename K, typename V> static inline std::ostream &operator<<(std::ostream &os, const map<K, V> &val) {
  return os << repr(val, Constants::jsonTrace);
}
template <typename K, typename V, typename C>
static inline std::ostream &operator<<(std::ostream &os, const map<K, V, C> &val) {
  return os << repr(val, Constants::jsonTrace);
}

template <typename K, typename V> static inline std::ostream &operator<<(std::ostream &os, const multimap<K, V> &val) {
  return os << repr(val, Constants::jsonTrace);
}

template <typename F, typename S> static inline std::ostream &operator<<(std::ostream &os, const pair<F, S> &val) {
  return os << repr(val, Constants::jsonTrace);
}

template <typename T> static inline std::ostream &operator<<(std::ostream &os, const set<T> &val) {
  return os << repr(val, Constants::jsonTrace);
}

static inline std::ostream &operator<<(std::ostream &os, const struct pollfd &val) {
  return os << repr(val, Constants::jsonTrace);
}
static inline std::ostream &operator<<(std::ostream &os, const HttpServer::CgiProcess &val) {
  return os << repr(val, Constants::jsonTrace);
}

// kinda belongs into Logger, but can't do that because it makes things SUPER ugly (circular dependencies and so on)
// extern stuff
#include <string>

#include "MacroMagic.h"

#define TRACE_COPY_ASSIGN_OP                                                                                           \
  do {                                                                                                                 \
    Logger::StreamWrapper oss = log.trace();                                                                           \
    if (Constants::jsonTrace)                                                                                          \
      oss << "{\"event\":\"copy assignment operator\",\"other object\":" << ::repr(other) << "}\n";                    \
    else                                                                                                               \
      oss << kwrd(getClass(*this)) + punct("& ") + kwrd(getClass(*this)) + punct("::") + func("operator") +            \
                 punct("=(")                                                                                           \
          << ::repr(other) << punct(")") + '\n';                                                                       \
  } while (false)

#define TRACE_COPY_CTOR                                                                                                \
  do {                                                                                                                 \
    Logger::StreamWrapper oss = log.trace();                                                                           \
    if (Constants::jsonTrace)                                                                                          \
      oss << "{\"event\":\"copy constructor\",\"other object\":" << ::repr(other)                                      \
          << ",\"this object\":" << ::repr(*this) << "}\n";                                                            \
    else                                                                                                               \
      oss << kwrd(getClass(*this)) + punct("(") << ::repr(other) << punct(") -> ") << ::repr(*this) << '\n';           \
  } while (false)

#define GEN_NAMES_FIRST(type, name) << #type << " " << #name

#define GEN_NAMES(type, name) << punct(", ") << #type << " " << #name

#define GEN_REPRS_FIRST(_, name)                                                                                       \
  << (Constants::kwargLogs ? cmt(#name) : "") << (Constants::kwargLogs ? cmt("=") : "") << ::repr(name)

#define GEN_REPRS(_, name)                                                                                             \
  << punct(", ") << (Constants::kwargLogs ? cmt(#name) : "") << (Constants::kwargLogs ? cmt("=") : "") << ::repr(name)

#define TRACE_ARG_CTOR(...)                                                                                            \
  do {                                                                                                                 \
    Logger::StreamWrapper oss = log.trace();                                                                           \
    if (Constants::jsonTrace)                                                                                          \
      IF(IS_EMPTY(__VA_ARGS__))                                                                                        \
    (oss << "{\"event\":\"default constructor\",\"this object\":" << ::repr(*this) << "}\n";                           \
     , oss << "{\"event\":\"(" EXPAND(DEFER(GEN_NAMES_FIRST)(HEAD2(__VA_ARGS__)))                                      \
                  FOR_EACH_PAIR(GEN_NAMES, TAIL2(__VA_ARGS__))                                                         \
           << ") constructor\",\"this object\":" << ::repr(*this)                                                      \
           << "}\n";) else IF(IS_EMPTY(__VA_ARGS__))(oss << kwrd(getClass(*this)) + punct("() -> ") << ::repr(*this)   \
                                                         << '\n';                                                      \
                                                     , oss << kwrd(getClass(*this)) +                                  \
                                                                  punct("(") EXPAND(                                   \
                                                                      DEFER(GEN_REPRS_FIRST)(HEAD2(__VA_ARGS__)))      \
                                                                      FOR_EACH_PAIR(GEN_REPRS, TAIL2(__VA_ARGS__))     \
                                                           << punct(") -> ") << ::repr(*this) << '\n';)                \
  } while (false)

#define TRACE_DEFAULT_CTOR TRACE_ARG_CTOR()

#define TRACE_DTOR                                                                                                     \
  do {                                                                                                                 \
    Logger::StreamWrapper oss = log.trace();                                                                           \
    if (Constants::jsonTrace)                                                                                          \
      oss << "{\"event\":\"destructor\",\"this object\":" << ::repr(*this) << "}\n";                                   \
    else                                                                                                               \
      oss << punct("~") << ::repr(*this) << '\n';                                                                      \
  } while (false)

#define TRACE_SWAP_BEGIN                                                                                               \
  do {                                                                                                                 \
    Logger::StreamWrapper oss = log.trace();                                                                           \
    if (Constants::jsonTrace) {                                                                                        \
      oss << "{\"event\":\"object swap\",\"this object\":" << ::repr(*this) << ",\"other object\":" << ::repr(other)   \
          << "}\n";                                                                                                    \
    } else {                                                                                                           \
      oss << cmt("<Swapping " + std::string(getClass(*this)) + " *this:") + '\n';                                      \
      oss << ::repr(*this) << '\n';                                                                                    \
      oss << cmt("with the following" + std::string(getClass(*this)) + "object:") + '\n';                              \
      oss << ::repr(other) << '\n';                                                                                    \
    }                                                                                                                  \
  } while (false)

#define TRACE_SWAP_END                                                                                                 \
  do {                                                                                                                 \
    Logger::StreamWrapper oss = log.trace();                                                                           \
    if (!Constants::jsonTrace)                                                                                         \
      oss << cmt(std::string(getClass(*this)) + " swap done>") + '\n';                                                 \
  } while (false)
