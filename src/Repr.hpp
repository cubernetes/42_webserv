#pragma once /* Repr.hpp */
// This file MUST be included AFTER all of the following includes (to avoid circular
// dependencies)
// - HttpServer.hpp
// - CgiHandler.hpp

#include <ctime>
#include <deque>
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
#include "Logger.hpp"
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
using std::deque;
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
    static inline string repr(const T &value) { return value.repr(); }
};

template <class T> static inline string getClass(const T &v) {
    (void)v;
    return "(unknown)";
}

// convenience wrapper
template <typename T> static inline string repr(const T &value) {
    return ReprWrapper<T>::repr(value);
}

// convenience wrapper for arrays with size
template <typename T> static inline string reprArr(const T *value, size_t size) {
    std::ostringstream oss;
    if (Logger::lastInstance().istrace5())
        oss << "[";
    else if (Logger::lastInstance().istrace4())
        oss << punct("{");
    else
        oss << punct("[");
    for (size_t i = 0; i < size; ++i) {
        if (i != 0) {
            if (Logger::lastInstance().istrace5())
                oss << ", ";
            else
                oss << punct(", ");
        }
        oss << ReprWrapper<T>::repr(value[i]);
    }
    if (Logger::lastInstance().istrace5())
        oss << "]";
    else if (Logger::lastInstance().istrace4())
        oss << punct("}");
    else
        oss << punct("]");
    return oss.str();
}

///// repr partial template specializations

#define INT_REPR(T)                                                                      \
    template <> struct ReprWrapper<T> {                                                  \
        static inline string repr(const T &value) {                                      \
            std::ostringstream oss;                                                      \
            oss << value;                                                                \
            if (Logger::lastInstance().istrace5())                                       \
                return oss.str();                                                        \
            else                                                                         \
                return num(oss.str());                                                   \
        }                                                                                \
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
    static inline string repr(const bool &value) {
        if (Logger::lastInstance().istrace5())
            return value ? "true" : "false";
        else
            return num(value ? "true" : "false");
    }
};

template <> struct ReprWrapper<string> {
    static inline string repr(const string &value) {
        if (Logger::lastInstance().istrace5())
            return "\"" + Utils::jsonEscape(value) + "\"";
        else
            return str("\"" + Utils::jsonEscape(value) + "\"") +
                   (Logger::lastInstance().istrace4() ? punct("s") : "");
    }
};

// print generic pointers
template <typename T> struct ReprWrapper<T *> {
    static inline string repr(const T *const &value) {
        std::ostringstream oss;
        if (value)
            oss << value;
        else
            oss << num("NULL");
        if (Logger::lastInstance().istrace5())
            return oss.str();
        else
            return num(oss.str());
    }
};

template <> struct ReprWrapper<char *> {
    static inline string repr(const char *const &value) {
        if (Logger::lastInstance().istrace5()) {
            if (value)
                return "\"" + Utils::jsonEscape(value) + "\"";
            else
                return ReprWrapper<const void *>::repr(value);
        } else {
            if (value)
                return str("\"" + Utils::jsonEscape(value) + "\"");
            else
                return ReprWrapper<const void *>::repr(value);
        }
    }
};

// TODO: @timo: escape for char literal
#define CHAR_REPR(T)                                                                     \
    template <> struct ReprWrapper<T> {                                                  \
        static inline string repr(const T &value) {                                      \
            if (Logger::lastInstance().istrace5())                                       \
                return string("\"") + Utils::jsonEscape(string(1, (char)value)) + "\"";  \
            else                                                                         \
                return chr(string("'") +                                                 \
                           (value == '\\'   ? "\\\\"                                     \
                            : value == '\'' ? "\\'"                                      \
                                            : string(1, (char)value)) +                  \
                           "'");                                                         \
        }                                                                                \
    }

CHAR_REPR(char);
CHAR_REPR(unsigned char);
CHAR_REPR(signed char);

#define MAKE_MEMBER_INIT_LIST(_, name) , name()
#define MAKE_DECL(type, name) type name;
#define MAKE_REPR_FN(_, name)                                                            \
    string CAT(repr_, name)() const { return ::repr(name); }
#define MAKE_ASSIGN_GETTER(_, name) singleton.name = value.CAT(get, name)();
#define MAKE_ASSIGN_MEMBER(_, name) singleton.name = value.name;
#define MAKE_REFLECT(_, name)                                                            \
    members[#name] =                                                                     \
        std::make_pair((ReprClosure) & ReprWrapper::CAT(repr_, name), &singleton.name);
#define POST_REFLECT_GETTER(clsId, ...)                                                  \
    static inline string getClass(const clsId &v) {                                      \
        (void)v;                                                                         \
        return #clsId;                                                                   \
    }                                                                                    \
    template <> struct ReprWrapper<clsId> : public Reflection {                          \
        void reflect() {}                                                                \
        ReprWrapper()                                                                    \
            : uniqueNameMustComeFirst()                                                  \
                  FOR_EACH_PAIR(MAKE_MEMBER_INIT_LIST, __VA_ARGS__) {}                   \
        int uniqueNameMustComeFirst;                                                     \
        FOR_EACH_PAIR(MAKE_DECL, __VA_ARGS__)                                            \
        FOR_EACH_PAIR(MAKE_REPR_FN, __VA_ARGS__)                                         \
        static inline string repr(const clsId &value) {                                  \
            (void)value;                                                                 \
            static ReprWrapper<clsId> singleton;                                         \
            FOR_EACH_PAIR(MAKE_ASSIGN_GETTER, __VA_ARGS__)                               \
            Members members;                                                             \
            FOR_EACH_PAIR(MAKE_REFLECT, __VA_ARGS__)                                     \
            return singleton.reprStruct(#clsId, members);                                \
        }                                                                                \
    }
#define POST_REFLECT_MEMBER(clsId, ...)                                                  \
    static inline string getClass(const clsId &v) {                                      \
        (void)v;                                                                         \
        return #clsId;                                                                   \
    }                                                                                    \
    template <> struct ReprWrapper<clsId> : public Reflection {                          \
        void reflect() {}                                                                \
        ReprWrapper()                                                                    \
            : uniqueNameMustComeFirst()                                                  \
                  FOR_EACH_PAIR(MAKE_MEMBER_INIT_LIST, __VA_ARGS__) {}                   \
        ReprWrapper(const ReprWrapper &other)                                            \
            : Reflection(other), uniqueNameMustComeFirst()                               \
                                     FOR_EACH_PAIR(MAKE_MEMBER_INIT_LIST, __VA_ARGS__) { \
        }                                                                                \
        ReprWrapper &operator=(const ReprWrapper &) { return *this; }                    \
        int uniqueNameMustComeFirst;                                                     \
        FOR_EACH_PAIR(MAKE_DECL, __VA_ARGS__)                                            \
        FOR_EACH_PAIR(MAKE_REPR_FN, __VA_ARGS__)                                         \
        static inline string repr(const clsId &value) {                                  \
            (void)value;                                                                 \
            static ReprWrapper<clsId> singleton;                                         \
            FOR_EACH_PAIR(MAKE_ASSIGN_MEMBER, __VA_ARGS__)                               \
            Members members;                                                             \
            FOR_EACH_PAIR(MAKE_REFLECT, __VA_ARGS__)                                     \
            return singleton.reprStruct(#clsId, members);                                \
        }                                                                                \
    }

POST_REFLECT_MEMBER(struct pollfd, int, fd, struct pollevents_helper, events,
                    struct pollevents_helper, revents);

#include "HttpServer.hpp"
POST_REFLECT_MEMBER(HttpServer::Server, struct in_addr, ip, struct in_port_t_helper, port,
                    vector<string>, serverNames, Directives, directives, LocationCtxs,
                    locations);
POST_REFLECT_MEMBER(HttpServer::CgiProcess, pid_t, pid, int, readFd, int, writeFd, string,
                    response, unsigned long, totalSize, int, clientSocket,
                    const LocationCtx *, location, bool, headersSent, std::time_t,
                    lastActive);
POST_REFLECT_MEMBER(HttpServer::HttpRequest, string, method, string, path, string,
                    httpVersion, HttpServer::Headers, headers, string, body,
                    HttpServer::RequestState, state, size_t, contentLength, bool,
                    chunkedTransfer, size_t, bytesRead, string, temporaryBuffer, bool,
                    pathParsed);
POST_REFLECT_MEMBER(HttpServer::MultPlexFds, MultPlexType, multPlexType,
                    HttpServer::SelectFds, selectFds, HttpServer::PollFds, pollFds,
                    HttpServer::EpollFds, epollFds, HttpServer::FdStates, fdStates);
POST_REFLECT_GETTER(
    HttpServer, HttpServer::MultPlexFds, _monitorFds, HttpServer::ClientFdToCgiMap,
    _clientToCgi, HttpServer::CgiFdToClientMap, _cgiToClient, vector<int>,
    _listeningSockets, HttpServer::PollFds, _pollFds, string, _httpVersionString,
    HttpServer::PendingWriteMap, _pendingWrites, HttpServer::PendingCloses,
    _pendingCloses, HttpServer::DefaultServers, _defaultServers,
    HttpServer::PendingRequests,
    _pendingRequests); /*, HttpServer::StatusTexts, _statusTexts, HttpServer::MimeTypes,
                          _mimeTypes, Config, _config, HttpServer::Servers, _servers,
                          string, _rawConfig); */

#include "CgiHandler.hpp"
POST_REFLECT_GETTER(CgiHandler, string, _extension, string, _program);

// for vector
template <typename T> struct ReprWrapper<vector<T> > {
    static inline string repr(const vector<T> &value) {
        std::ostringstream oss;
        if (Logger::lastInstance().istrace5())
            oss << "[";
        else if (Logger::lastInstance().istrace4())
            oss << kwrd("std") + punct("::") + kwrd("vector") + punct("({");
        else
            oss << punct("[");
        for (size_t i = 0; i < value.size(); ++i) {
            if (i != 0) {
                if (Logger::lastInstance().istrace5())
                    oss << ", ";
                else
                    oss << punct(", ");
            }
            oss << ReprWrapper<T>::repr(value[i]);
        }
        if (Logger::lastInstance().istrace5())
            oss << "]";
        else if (Logger::lastInstance().istrace4())
            oss << punct("})");
        else
            oss << punct("]");
        return oss.str();
    }
};

template <typename T> struct ReprWrapper<deque<T> > {
    static inline string repr(const deque<T> &value) {
        std::ostringstream oss;
        if (Logger::lastInstance().istrace5())
            oss << "[";
        else if (Logger::lastInstance().istrace4())
            oss << kwrd("std") + punct("::") + kwrd("deque") + punct("({");
        else
            oss << punct("[");
        for (size_t i = 0; i < value.size(); ++i) {
            if (i != 0) {
                if (Logger::lastInstance().istrace5())
                    oss << ", ";
                else
                    oss << punct(", ");
            }
            oss << ReprWrapper<T>::repr(value[i]);
        }
        if (Logger::lastInstance().istrace5())
            oss << "]";
        else if (Logger::lastInstance().istrace4())
            oss << punct("})");
        else
            oss << punct("]");
        return oss.str();
    }
};

// for map
template <typename K, typename V> struct ReprWrapper<map<K, V> > {
    static inline string repr(const map<K, V> &value) {
        std::ostringstream oss;
        if (Logger::lastInstance().istrace5())
            oss << "{";
        else if (Logger::lastInstance().istrace4())
            oss << kwrd("std") + punct("::") + kwrd("map") + punct("({");
        else
            oss << punct("{");
        int i = 0;
        for (typename map<K, V>::const_iterator it = value.begin(); it != value.end();
             ++it) {
            if (i != 0) {
                if (Logger::lastInstance().istrace5())
                    oss << ", ";
                else
                    oss << punct(", ");
            }
            oss << ReprWrapper<K>::repr(it->first);
            if (Logger::lastInstance().istrace5())
                oss << ": ";
            else
                oss << punct(": ");
            oss << ReprWrapper<V>::repr(it->second);
            ++i;
        }
        if (Logger::lastInstance().istrace5())
            oss << "}";
        else if (Logger::lastInstance().istrace4())
            oss << punct("})");
        else
            oss << punct("}");
        return oss.str();
    }
};

// for map with comparison function
template <typename K, typename V, typename C> struct ReprWrapper<map<K, V, C> > {
    static inline string repr(const map<K, V, C> &value) {
        std::ostringstream oss;
        if (Logger::lastInstance().istrace5())
            oss << "{";
        else if (Logger::lastInstance().istrace4())
            oss << kwrd("std") + punct("::") + kwrd("map") + punct("({");
        else
            oss << punct("{");
        int i = 0;
        for (typename map<K, V, C>::const_iterator it = value.begin(); it != value.end();
             ++it) {
            if (i != 0) {
                if (Logger::lastInstance().istrace5())
                    oss << ", ";
                else
                    oss << punct(", ");
            }
            oss << ReprWrapper<K>::repr(it->first);
            if (Logger::lastInstance().istrace5())
                oss << ": ";
            else
                oss << punct(": ");
            oss << ReprWrapper<V>::repr(it->second);
            ++i;
        }
        if (Logger::lastInstance().istrace5())
            oss << "}";
        else if (Logger::lastInstance().istrace4())
            oss << punct("})");
        else
            oss << punct("}");
        return oss.str();
    }
};

// for multimap
template <typename K, typename V> struct ReprWrapper<multimap<K, V> > {
    static inline string repr(const multimap<K, V> &value) {
        std::ostringstream oss;
        if (Logger::lastInstance().istrace5())
            oss << "{";
        else if (Logger::lastInstance().istrace4())
            oss << kwrd("std") + punct("::") + kwrd("multimap") + punct("({");
        else
            oss << punct("{");
        int i = 0;
        for (typename multimap<K, V>::const_iterator it = value.begin();
             it != value.end(); ++it) {
            if (i != 0) {
                if (Logger::lastInstance().istrace5())
                    oss << ", ";
                else
                    oss << punct(", ");
            }
            oss << ReprWrapper<K>::repr(it->first);
            if (Logger::lastInstance().istrace5())
                oss << ": ";
            else
                oss << punct(": ");
            oss << ReprWrapper<V>::repr(it->second);
            ++i;
        }
        if (Logger::lastInstance().istrace5())
            oss << "}";
        else if (Logger::lastInstance().istrace4())
            oss << punct("})");
        else
            oss << punct("}");
        return oss.str();
    }
};

// for pair
template <typename F, typename S> struct ReprWrapper<pair<F, S> > {
    static inline string repr(const pair<F, S> &value) {
        std::ostringstream oss;
        if (Logger::lastInstance().istrace5())
            oss << "[";
        else if (Logger::lastInstance().istrace4())
            oss << kwrd("std") + punct("::") + kwrd("pair") + punct("(");
        else
            oss << punct("(");
        oss << ReprWrapper<F>::repr(value.first)
            << (Logger::lastInstance().istrace5() ? ", " : punct(", "))
            << ReprWrapper<S>::repr(value.second);
        if (Logger::lastInstance().istrace5())
            oss << "]";
        else
            oss << punct(")");
        return oss.str();
    }
};

// for set
template <typename T> struct ReprWrapper<set<T> > {
    static inline string repr(const set<T> &value) {
        std::ostringstream oss;
        if (Logger::lastInstance().istrace5())
            oss << "[";
        else if (Logger::lastInstance().istrace4())
            oss << kwrd("std") + punct("::") + kwrd("set") + punct("({");
        else
            oss << punct("{");
        int i = -1;
        for (typename set<T>::const_iterator it = value.begin(); it != value.end();
             ++it) {
            if (++i != 0) {
                if (Logger::lastInstance().istrace5())
                    oss << ", ";
                else
                    oss << punct(", ");
            }
            oss << ReprWrapper<T>::repr(*it);
        }
        if (Logger::lastInstance().istrace5())
            oss << "]";
        else if (Logger::lastInstance().istrace4())
            oss << punct("})");
        else
            oss << punct("}");
        return oss.str();
    }
};

// for struct in_addr
template <> struct ReprWrapper<struct in_addr> {
    static inline string repr(const struct in_addr &value) {
        std::ostringstream oss;
        if (Logger::lastInstance().istrace5()) {
            oss << value.s_addr;
            return oss.str();
        }
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
        std::ostringstream oss_final;
        oss << (int)addr.first << '.';
        oss << (int)addr.second << '.';
        oss << (int)addr.third << '.';
        oss << (int)addr.fourth;
        if (Logger::lastInstance().istrace4())
            oss_final << kwrd("struct in_addr") << punct("(");
        oss_final << num(oss.str());
        if (Logger::lastInstance().istrace4())
            oss_final << punct(")");
        return oss_final.str();
    }
};

POST_REFLECT_MEMBER(struct sockaddr_in, struct in_addr, sin_addr, in_port_t, sin_port);

// for struct in_port_t_helper
template <> struct ReprWrapper<struct in_port_t_helper> {
    static inline string repr(const struct in_port_t_helper &value) {
        std::ostringstream oss;
        oss << ntohs(value.port);
        if (Logger::lastInstance().istrace5())
            return oss.str();
        else
            return num(oss.str());
    }
};

// for struct sockaddr_in_helper
template <> struct ReprWrapper<struct sockaddr_in_wrapper> {
    static inline string repr(const struct sockaddr_in_wrapper &value) {
        std::ostringstream oss;
        if (Logger::lastInstance().istrace5()) {
            struct sockaddr_in saddr;
            saddr.sin_addr = value.sin_addr;
            saddr.sin_port = value.sin_port;
            return ReprWrapper<struct sockaddr_in>::repr(saddr);
        }
        if (Logger::lastInstance().istrace4())
            oss << kwrd("struct sockaddr_in") << punct("(");
        oss << ReprWrapper<struct in_addr>::repr(value.sin_addr) << num(":")
            << ReprWrapper<in_port_t_helper>::repr(in_port_t_helper(value.sin_port));
        if (Logger::lastInstance().istrace4())
            oss << punct(")");
        return oss.str();
    }
};

static void pollevents_helper_adder(short event, const string &eventName, short events,
                                    bool &atLeastOne, std::ostringstream &oss) {
    if (events & event) {
        if (atLeastOne)
            oss << punct(", ");
        oss << num(eventName);
        atLeastOne = true;
    }
}

// for struct pollevents_helper
template <> struct ReprWrapper<struct pollevents_helper> {
    static inline string repr(const struct pollevents_helper &value) {
        std::ostringstream oss;
        short events = value.events;
        if (Logger::lastInstance().istrace5()) {
            oss << events;
            return oss.str();
        }
        bool atLeastOne = false;
        pollevents_helper_adder(POLLIN, "POLLIN", events, atLeastOne, oss);
        pollevents_helper_adder(POLLPRI, "POLLPRI", events, atLeastOne, oss);
        pollevents_helper_adder(POLLOUT, "POLLOUT", events, atLeastOne, oss);
        pollevents_helper_adder(POLLRDHUP, "POLLRDHUP", events, atLeastOne, oss);
        pollevents_helper_adder(POLLERR, "POLLERR", events, atLeastOne, oss);
        pollevents_helper_adder(POLLHUP, "POLLHUP", events, atLeastOne, oss);
        pollevents_helper_adder(POLLNVAL, "POLLNVAL", events, atLeastOne, oss);
        string all = oss.str();
        if (all.empty())
            return num("NO_EVENTS");
        return punct("[") + all + punct("]");
    }
};

#include <sys/epoll.h>
// for struct epoll_event
template <> struct ReprWrapper<struct epoll_event> {
    static inline string repr(const struct epoll_event &value) {
        (void)value;
        std::ostringstream oss;
        oss << "epoll_event(...)";
        if (Logger::lastInstance().istrace5())
            return oss.str();
        else
            return kwrd(oss.str());
    }
};

// for enum MultPlexType
template <> struct ReprWrapper<MultPlexType> {
    static inline string repr(const MultPlexType &value) {
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
        if (Logger::lastInstance().istrace5())
            return oss.str();
        else
            return num(oss.str());
    }
};

// for enum RequestState
template <> struct ReprWrapper<HttpServer::RequestState> {
    static inline string repr(const HttpServer::RequestState &value) {
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
        if (Logger::lastInstance().istrace5())
            return oss.str();
        else
            return num(oss.str());
    }
};

// for enum FdState
template <> struct ReprWrapper<HttpServer::FdState> {
    static inline string repr(const HttpServer::FdState &value) {
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
        if (Logger::lastInstance().istrace5())
            return oss.str();
        else
            return num(oss.str());
    }
};

// for enum TokenType
template <> struct ReprWrapper<TokenType> {
    static inline string repr(const TokenType &value) {
        std::ostringstream oss;
        switch (value) {
        case TOK_SEMICOLON:
            oss << "TOK_SEMICOLON";
            break;
        case TOK_OPENING_BRACE:
            oss << "TOK_OPENING_BRACE";
            break;
        case TOK_CLOSING_BRACE:
            oss << "TOK_CLOSING_BRACE";
            break;
        case TOK_WORD:
            oss << "TOK_WORD";
            break;
        case TOK_EOF:
            oss << "TOK_EOF";
            break;
        case TOK_UNKNOWN:
            oss << "TOK_UNKNOWN";
            break;
        default:
            oss << "UNKNOWN_TOKEN";
            break;
        }
        if (Logger::lastInstance().istrace5())
            return oss.str();
        else
            return num(oss.str());
    }
};

// to print using `std::cout << ...'
template <typename T>
static inline std::ostream &operator<<(std::ostream &os, const vector<T> &val) {
    return os << repr(val);
}

template <typename T>
static inline std::ostream &operator<<(std::ostream &os, const deque<T> &val) {
    return os << repr(val);
}

template <typename K, typename V>
static inline std::ostream &operator<<(std::ostream &os, const map<K, V> &val) {
    return os << repr(val);
}
template <typename K, typename V, typename C>
static inline std::ostream &operator<<(std::ostream &os, const map<K, V, C> &val) {
    return os << repr(val);
}

template <typename K, typename V>
static inline std::ostream &operator<<(std::ostream &os, const multimap<K, V> &val) {
    return os << repr(val);
}

template <typename F, typename S>
static inline std::ostream &operator<<(std::ostream &os, const pair<F, S> &val) {
    return os << repr(val);
}

template <typename T>
static inline std::ostream &operator<<(std::ostream &os, const set<T> &val) {
    return os << repr(val);
}

static inline std::ostream &operator<<(std::ostream &os, const struct pollfd &val) {
    return os << repr(val);
}

static inline std::ostream &operator<<(std::ostream &os, const struct sockaddr_in &val) {
    return os << repr(val);
}

static inline std::ostream &operator<<(std::ostream &os,
                                       const HttpServer::CgiProcess &val) {
    return os << repr(val);
}

// kinda belongs into Logger, but can't do that because it makes things SUPER ugly
// (circular dependencies and so on) extern stuff
#include <string>

#include "MacroMagic.h"

#define CURRENT_TRACE_STREAM                                                             \
    (log.istrace5()                                                                      \
         ? log.trace5()                                                                  \
         : (log.istrace4()                                                               \
                ? log.trace4()                                                           \
                : (log.istrace3() ? log.trace3()                                         \
                                  : (log.istrace2() ? log.trace2()                       \
                                                    : (log.istrace() ? log.trace()       \
                                                                     : log.trace())))))

#define TRACE_COPY_ASSIGN_OP                                                             \
    do {                                                                                 \
        Logger::StreamWrapper &oss = CURRENT_TRACE_STREAM;                               \
        oss << "Changing object via copy assignment operator: ";                         \
        if (log.istrace5())                                                              \
            oss << "{\"event\":\"copy assignment operator\",\"other object\":"           \
                << ::repr(other) << "}\n";                                               \
        else if (log.istrace2())                                                         \
            oss << kwrd(getClass(*this)) + punct("& ") + kwrd(getClass(*this)) +         \
                       punct("::") + func("operator") + punct("=(")                      \
                << ::repr(other) << punct(")") + '\n';                                   \
        else                                                                             \
            oss << func("operator") + punct("=(const " + kwrd(getClass(*this)) + "&)") + \
                       '\n';                                                             \
    } while (false)

#define TRACE_COPY_CTOR                                                                  \
    do {                                                                                 \
        Logger::StreamWrapper &oss = CURRENT_TRACE_STREAM;                               \
        oss << "Creating object via copy constructor: ";                                 \
        if (log.istrace5())                                                              \
            oss << "{\"event\":\"copy constructor\",\"other object\":" << ::repr(other)  \
                << ",\"this object\":" << ::repr(*this) << "}\n";                        \
        else if (log.istrace2())                                                         \
            oss << kwrd(getClass(*this)) + punct("::") + kwrd(getClass(*this)) +         \
                       punct("(")                                                        \
                << ::repr(other) << punct(") -> ") << ::repr(*this) << '\n';             \
        else                                                                             \
            oss << kwrd(getClass(*this)) + punct("(const ") << kwrd(getClass(*this))     \
                << punct("&)") << '\n';                                                  \
    } while (false)

#define GEN_NAMES_FIRST(type, name) << #type << " " << #name

#define GEN_NAMES(type, name)                                                            \
    << (log.istrace5() ? ", " : punct(", ")) << #type << " " << #name

#define GEN_REPRS_FIRST(type, name)                                                      \
    << (log.istrace2() ? (log.istrace3() ? cmt(#name) : "") +                            \
                             (log.istrace3() ? cmt("=") : "") + ::repr(name)             \
                       : cmt(std::string(#type) + " " + #name))

#define GEN_REPRS(type, name)                                                            \
    << (log.istrace2() ? punct(", ") + (log.istrace3() ? cmt(#name) : "") +              \
                             (log.istrace3() ? cmt("=") : "") + ::repr(name)             \
                       : cmt(std::string(", ") + #type + " " + #name))

#define TRACE_ARG_CTOR(...)                                                              \
    do {                                                                                 \
        Logger::StreamWrapper &oss = CURRENT_TRACE_STREAM;                               \
        IF(IS_EMPTY(__VA_ARGS__))                                                        \
        (oss << "Creating object via default constructor: ",                             \
         oss << "Creating object via " << NARG(__VA_ARGS__) / 2                          \
             << "-ary constructor: ");                                                   \
        if (log.istrace5())                                                              \
            IF(IS_EMPTY(__VA_ARGS__))                                                    \
        (oss << "{\"event\":\"default constructor\",\"this object\":" << ::repr(*this)   \
             << "}\n",                                                                   \
         oss << "{\"event\":\"(" EXPAND(DEFER(GEN_NAMES_FIRST)(HEAD2(__VA_ARGS__)))      \
                    FOR_EACH_PAIR(GEN_NAMES, TAIL2(__VA_ARGS__))                         \
             << ") constructor\",\"this object\":" << ::repr(*this) << "}\n");           \
        else if (log.istrace2()) IF(IS_EMPTY(__VA_ARGS__))(                              \
            oss << kwrd(getClass(*this)) + punct("() -> ") << ::repr(*this) << '\n',     \
            oss << kwrd(getClass(*this)) +                                               \
                       punct("(") EXPAND(DEFER(GEN_REPRS_FIRST)(HEAD2(__VA_ARGS__)))     \
                           FOR_EACH_PAIR(GEN_REPRS, TAIL2(__VA_ARGS__))                  \
                << punct(") -> ") << ::repr(*this) << '\n');                             \
        else IF(IS_EMPTY(__VA_ARGS__))(                                                  \
            oss << kwrd(getClass(*this)) + punct("()") << '\n',                          \
            oss << kwrd(getClass(*this)) +                                               \
                       punct("(") EXPAND(DEFER(GEN_REPRS_FIRST)(HEAD2(__VA_ARGS__)))     \
                           FOR_EACH_PAIR(GEN_REPRS, TAIL2(__VA_ARGS__))                  \
                << punct(")") << '\n');                                                  \
    } while (false)

#define TRACE_DEFAULT_CTOR TRACE_ARG_CTOR()

#define TRACE_DTOR                                                                       \
    do {                                                                                 \
        Logger::StreamWrapper &oss = CURRENT_TRACE_STREAM;                               \
        oss << "Destructing object: ";                                                   \
        if (log.istrace5())                                                              \
            oss << "{\"event\":\"destructor\",\"this object\":" << ::repr(*this)         \
                << "}\n";                                                                \
        else if (log.istrace2())                                                         \
            oss << punct("~") << ::repr(*this) << '\n';                                  \
        else                                                                             \
            oss << punct("~") + kwrd(getClass(*this)) + punct("()") << '\n';             \
    } while (false)

#define TRACE_SWAP_BEGIN                                                                 \
    do {                                                                                 \
        Logger::StreamWrapper &oss = CURRENT_TRACE_STREAM;                               \
        oss << "Starting swap operation: ";                                              \
        if (log.istrace5()) {                                                            \
            oss << "{\"event\":\"object swap\",\"this object\":" << ::repr(*this)        \
                << ",\"other object\":" << ::repr(other) << "}\n";                       \
        } else if (log.istrace2()) {                                                     \
            oss << cmt("<Swapping " + std::string(getClass(*this)) + " *this:") + '\n';  \
            oss << ::repr(*this) << '\n';                                                \
            oss << cmt("with the following" + std::string(getClass(*this)) +             \
                       "object:") +                                                      \
                       '\n';                                                             \
            oss << ::repr(other) << '\n';                                                \
        } else {                                                                         \
            oss << cmt("<Swapping " + std::string(getClass(*this)) + " with another " +  \
                       std::string(getClass(*this))) +                                   \
                       '\n';                                                             \
        }                                                                                \
    } while (false)

#define TRACE_SWAP_END                                                                   \
    do {                                                                                 \
        Logger::StreamWrapper &oss = CURRENT_TRACE_STREAM;                               \
        if (!log.istrace5())                                                             \
            oss << cmt(std::string(getClass(*this)) + " swap done>") + '\n';             \
    } while (false)
