#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "Constants.hpp"
#include "DirectiveValidation.hpp"
#include "Errors.hpp"
#include "Logger.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

#define CHECKFN(ctx, checkFunction)                                                      \
    if (checkFunction(ctx, directive, arguments) && ++counts[directive] <= 1)            \
    continue

#define CHECKFN_MULTI(ctx, checkFunction)                                                \
    if (checkFunction(ctx, directive, arguments))                                        \
    continue

using std::map;
using std::pair;
using std::runtime_error;
using std::string;
using std::vector;

// (max < 0 -> no maximum
static inline void ensureArity(const string &ctx, const string &directive,
                               const Arguments &arguments, int min, int max) {
    int ac = static_cast<int>(arguments.size());
    if (ac >= min && (ac <= max || max < 0))
        return;
    throw runtime_error(
        Errors::Config::InvalidDirectiveArgumentCount(ctx, directive, ac, min, max));
}

static inline void ensureOneOfStrings(const string &ctx, const string &directive,
                                      const string &argument,
                                      const Arguments &possibilities) {
    if (std::find(possibilities.begin(), possibilities.end(), argument) ==
        possibilities.end()) {
        throw runtime_error(Errors::Config::InvalidDirectiveArgument(
            ctx, directive, argument, possibilities));
    }
}

static inline void ensureRootedPath(const string &ctx, const string &directive,
                                    const string &argument) {
    if (argument.empty() || argument[0] != '/')
        throw runtime_error(
            Errors::Config::DirectiveArgumentNotRootedPath(ctx, directive, argument));
}

static inline void ensureNotEmpty(const string &ctx, const string &directive,
                                  const string &argument) {
    if (argument.empty())
        throw runtime_error(Errors::Config::DirectiveArgumentEmpty(ctx, directive));
}

bool DirectiveValidation::evenNumberOfBackslashes(const string &str, size_t endingAt) {
    int count = 0;
    for (int i = static_cast<int>(endingAt); i >= 0; --i) {
        if (str[static_cast<size_t>(i)] == '\\')
            ++count;
        else
            break;
    }
    return count % 2 == 0;
}

bool DirectiveValidation::isDoubleQuoted(const string &str) {
    if (str[0] != '"')
        return false;
    size_t i;
    for (i = 1; i < str.length() - 1; ++i) {
        if (str[i] == '"' && evenNumberOfBackslashes(str, i - 1))
            return false;
    }
    if (str[i] != '"')
        return false;
    return true;
}

bool DirectiveValidation::isHttpUri(const string &str) {
    size_t len = Utils::isPrefix("http://", str)    ? 7
                 : Utils::isPrefix("https://", str) ? 8
                                                    : 0;
    if (len == 0)
        return false;
    string withoutScheme = str.substr(len);
    string domain = withoutScheme.substr(0, withoutScheme.find('/'));
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int success = getaddrinfo(domain.c_str(), NULL, &hints, &res) == 0;
    if (success)
        freeaddrinfo(res);
    return success;
}

static inline char escapeChar(const char c) {
    switch (c) {
    case '0':
        return '\0';
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 't':
        return '\t';
    case 'n':
        return '\n';
    case 'v':
        return '\v';
    case 'f':
        return '\f';
    case 'r':
        return '\r';
    default:
        return c;
    }
}

string DirectiveValidation::decodeDoubleQuotedString(const string &str) {
    string newStr;
    if (str[0] != '"')
        return newStr; // ignore error
    size_t i;
    for (i = 1; i < str.length() - 1; ++i) {
        if (str[i] == '"')
            return newStr; // ignore error
        if (str[i] == '\\') {
            newStr += escapeChar(str[i + 1]);
            i += 1;
        } else
            newStr += str[i];
    }
    if (str[i] != '"')
        return newStr; // ignore error
    return newStr;
}

static inline void ensureValidDoubleQuotedString(const string &ctx,
                                                 const string &directive,
                                                 const string &argument) {
    if (!DirectiveValidation::isDoubleQuoted(argument))
        throw runtime_error(Errors::Config::DirectiveArgumentInvalidDoubleQuotedString(
            ctx, directive, argument));
}

static inline void ensureValidHttpUri(const string &ctx, const string &directive,
                                      const string &argument) {
    if (!DirectiveValidation::isHttpUri(argument))
        throw runtime_error(Errors::Config::DirectiveArgumentInvalidHttpUri(
            ctx, directive,
            argument)); // TODO: normally, you'd use a regex to validate the domain, but
                        // hell nah we are not implementing a NFA from scratch
}

// was only needed for the int argument to the worker_processes directive, which is not
// needed anymore static inline void ensureInt(const string& ctx, const string& directive,
// const string& argument) { 	ensureNotEmpty(ctx, directive, argument);
// 	string::const_iterator c = argument.begin();
// 	while (c != argument.end() && std::isdigit(*c)) ++c;
// 	if (c == argument.end())
// 		return;
// 	throw runtime_error(Errors::Config::DirectiveArgumentNotNumeric(ctx, directive,
// argument));
// }

static inline void ensureValidSize(const string &ctx, const string &directive,
                                   const string &argument) {
    ensureNotEmpty(ctx, directive, argument);
    string::const_iterator c = argument.begin();
    while (c != argument.end() && std::isdigit(*c))
        ++c;
    if (c == argument.end())
        return;
    if (std::strchr("kmgt", std::tolower(*c)) && c + 1 == argument.end())
        return;
    throw runtime_error(
        Errors::Config::DirectiveInvalidSizeArgument(ctx, directive, argument));
}

static inline void ensureOnOff(const string &ctx, const string &directive,
                               string &argument) {
    if (argument == "on" || argument == "yes" || argument == "true") {
        argument = "on";
        return;
    }
    if (argument == "off" || argument == "no" || argument == "false") {
        argument = "off";
        return;
    }
    throw runtime_error(
        Errors::Config::DirectiveInvalidBooleanArgument(ctx, directive, argument));
}

// only allow official status codes, might even prune this list even further
// https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
static inline void ensureStatusCode(const string &ctx, const string &directive,
                                    const string &argument) {
    static const char *validCodesArr[] = {
        "100", "101", "102", "103", "200", "201", "202", "203", "204", "205", "206",
        "207", "208", "226", "300", "301", "302", "303", "304", "305", "306", "307",
        "308", "400", "401", "402", "403", "404", "405", "406", "407", "408", "409",
        "410", "411", "412", "413", "414", "415", "416", "417", "418", "421", "422",
        "423", "424", "425", "426", "428", "429", "431", "451", "500", "501", "502",
        "503", "504", "505", "506", "507", "508", "510", "511"};
    static const vector<string> validCodes(
        validCodesArr, validCodesArr + sizeof(validCodesArr) / sizeof(validCodesArr[0]));
    if (std::find(validCodes.begin(), validCodes.end(), argument) != validCodes.end())
        return;
    throw runtime_error(
        Errors::Config::DirectiveInvalidStatusCodeArgument(ctx, directive, argument));
}

static inline void ensureValidHost(const string &ctx, const string &directive,
                                   string &argument) {
    if (argument == "*") {
        argument = "0.0.0.0";
        return;
    }
    char buf[16];
    if (inet_aton(argument.c_str(), NULL) == 1)
        return;
    if (inet_pton(AF_INET6, argument.c_str(), &buf) == 1)
        throw runtime_error(Errors::Config::DirectiveIPv6NotSupported(ctx, directive));
    throw runtime_error(
        Errors::Config::DirectiveInvalidIpAddressArgument(ctx, directive, argument));
}

static inline void ensureValidPort(const string &ctx, const string &directive,
                                   const string &argument) {
    char *end;
    long l;

    errno = 0;
    l = strtol(argument.c_str(), &end, 10);
    if ((errno == ERANGE && l == LONG_MAX) || l > Constants::highestPort)
        throw runtime_error(
            Errors::Config::DirectiveArgumentPortNumberTooHigh(ctx, directive, argument));
    if ((errno == ERANGE && l == LONG_MIN) || l < 1)
        throw runtime_error(
            Errors::Config::DirectiveArgumentPortNumberTooLow(ctx, directive, argument));
    if (argument.empty() || *end != '\0')
        throw runtime_error(
            Errors::Config::DirectiveArgumentInvalidPort(ctx, directive, argument));
}

static inline bool checkIndex(const string &ctx, const string &directive,
                              const Arguments &arguments) {
    if (directive == "index") {
        ensureArity(ctx, directive, arguments, 1, -1);
        for (Arguments::const_iterator argument = arguments.begin();
             argument != arguments.end(); ++argument) {
            ensureNotEmpty(ctx, directive, *argument);
        }
        return true;
    }
    return false;
}

static inline bool checkRoot(const string &ctx, const string &directive,
                             const Arguments &arguments) {
    if (directive == "root") {
        ensureArity(ctx, directive, arguments, 1, 1);
        ensureNotEmpty(ctx, directive, arguments[0]);
        return true;
    }
    return false;
}

static inline Arguments processIPv4Address(const string &host) {
    string ipAddress;
    string port;
    Arguments newArgs;
    string::size_type pos;
    if ((pos = host.rfind(':')) != string::npos) {
        ipAddress = host.substr(0, pos);
        port = host.substr(pos + 1);
    } else if ((pos = host.find('.')) != string::npos) {
        ipAddress = host;
        port = Constants::defaultPort;
    } else {
        ipAddress = Constants::defaultAddress;
        port = host;
    }
    newArgs.push_back(ipAddress);
    newArgs.push_back(port);
    return newArgs;
}

static inline bool checkListen(const string &ctx, const string &directive,
                               Arguments &arguments) {
    if (directive == "listen") {
        ensureArity(ctx, directive, arguments, 1, 1);
        Arguments ipAndPort = processIPv4Address(arguments[0]);
        ensureValidHost(ctx, directive, ipAndPort[0]);
        ensureValidPort(ctx, directive, ipAndPort[1]);
        arguments = ipAndPort;
        return true;
    }
    return false;
}

static inline bool checkServerName(const string &ctx, const string &directive,
                                   const Arguments &arguments) {
    if (directive == "server_name") {
        ensureArity(ctx, directive, arguments, 1, -1);
        return true;
    }
    return false;
}

static inline bool checkClientMaxBodySize(const string &ctx, const string &directive,
                                          const Arguments &arguments) {
    if (directive == "client_max_body_size") {
        ensureArity(ctx, directive, arguments, 1, 1);
        ensureValidSize(ctx, directive, arguments[0]);
        return true;
    }
    return false;
}

static inline bool checkLimitExcept(const string &ctx, const string &directive,
                                    const Arguments &arguments) {
    if (directive == "limit_except") {
        ensureArity(ctx, directive, arguments, 1, -1);
        for (Arguments::const_iterator argument = arguments.begin();
             argument != arguments.end(); ++argument) {
            ensureOneOfStrings(
                ctx, directive, *argument,
                VEC(string, "GET", "HEAD", "POST", "DELETE", "PUT", "FTFT"));
        }
        return true;
    }
    return false;
}

static inline bool checkAlias(const string &ctx, const string &directive,
                              const Arguments &arguments) {
    if (directive == "alias") {
        ensureArity(ctx, directive, arguments, 1, 1);
        ensureRootedPath(ctx, directive, arguments[0]);
        return true;
    }
    return false;
}

static inline bool checkReturn(const string &ctx, const string &directive,
                               const Arguments &arguments) {
    if (directive == "return") {
        ensureArity(ctx, directive, arguments, 2, 2);
        ensureStatusCode(ctx, directive, arguments[0]);
        ensureNotEmpty(ctx, directive, arguments[1]);
        if (arguments[1][0] == '"')
            ensureValidDoubleQuotedString(ctx, directive, arguments[1]);
        else
            ensureValidHttpUri(ctx, directive, arguments[1]);
        return true;
    }
    return false;
}

static inline bool checkAutoindex(const string &ctx, const string &directive,
                                  Arguments &arguments) {
    if (directive == "autoindex") {
        ensureArity(ctx, directive, arguments, 1, 1);
        ensureOnOff(ctx, directive, arguments[0]);
        return true;
    }
    return false;
}

static inline bool checkErrorPage(const string &ctx, const string &directive,
                                  const Arguments &arguments) {
    if (directive == "error_page") {
        ensureArity(ctx, directive, arguments, 2, -1);
        size_t i;
        for (i = 0; i < arguments.size() - 1; ++i) {
            ensureStatusCode(ctx, directive, arguments[i]);
        }
        ensureRootedPath(ctx, directive, arguments[i]);
        return true;
    }
    return false;
}

static inline bool checkCgiExt(const string &ctx, const string &directive,
                               const Arguments &arguments) {
    if (directive == "cgi_ext") {
        ensureArity(ctx, directive, arguments, 1, 2);
        if (arguments.size() == 2)
            ensureRootedPath(ctx, directive, arguments[1]);
        return true;
    }
    return false;
}

static inline bool checkCgiDir(const string &ctx, const string &directive,
                               const Arguments &arguments) {
    if (directive == "cgi_dir") {
        ensureArity(ctx, directive, arguments, 1, 1);
        ensureRootedPath(ctx, directive, arguments[0]);
        return true;
    }
    return false;
}

static inline bool checkUploadDir(const string &ctx, const string &directive,
                                  const Arguments &arguments) {
    if (directive == "upload_dir") {
        ensureArity(ctx, directive, arguments, 1, 1);
        if (arguments[0].empty())
            return true;
        ensureRootedPath(ctx, directive, arguments[0]);
        return true;
    }
    return false;
}

static void checkHttpDirectives(Directives &directives) {
    map<string, unsigned int> counts;
    for (Directives::iterator kv = directives.begin(); kv != directives.end(); ++kv) {
        const string &directive = kv->first;
        Arguments &arguments = kv->second;
        CHECKFN("http", checkAutoindex);
        CHECKFN("http", checkCgiDir);
        CHECKFN_MULTI("http", checkCgiExt);
        CHECKFN("http", checkClientMaxBodySize);
        CHECKFN_MULTI("http", checkErrorPage);
        CHECKFN_MULTI("http", checkIndex);
        CHECKFN("http", checkRoot);
        CHECKFN("http", checkUploadDir);
        if (counts[directive] > 1)
            throw runtime_error(Errors::Config::DirectiveNotUnique("http", directive));
        throw runtime_error(Errors::Config::UnknownDirective("http", directive));
    }
}

static void checkServerDirectives(Directives &directives) {
    map<string, unsigned int> counts;
    for (Directives::iterator kv = directives.begin(); kv != directives.end(); ++kv) {
        const string &directive = kv->first;
        Arguments &arguments = kv->second;
        CHECKFN("server", checkAutoindex);
        CHECKFN("server", checkCgiDir);
        CHECKFN_MULTI("server", checkCgiExt);
        CHECKFN("server", checkClientMaxBodySize);
        CHECKFN_MULTI("server", checkErrorPage);
        CHECKFN_MULTI("server", checkIndex);
        CHECKFN("server", checkListen);
        CHECKFN("server", checkRoot);
        CHECKFN("server", checkServerName);
        CHECKFN("server", checkUploadDir);
        if (counts[directive] > 1)
            throw runtime_error(Errors::Config::DirectiveNotUnique("server", directive));
        throw runtime_error(Errors::Config::UnknownDirective("server", directive));
    }
}

static void checkLocationDirectives(Directives &directives) {
    map<string, unsigned int> counts;
    for (Directives::iterator kv = directives.begin(); kv != directives.end(); ++kv) {
        const string &directive = kv->first;
        Arguments &arguments = kv->second;
        CHECKFN("location", checkAlias);
        CHECKFN("location", checkAutoindex);
        CHECKFN("location", checkCgiDir);
        CHECKFN_MULTI("location", checkCgiExt);
        CHECKFN("location", checkClientMaxBodySize);
        CHECKFN_MULTI("location", checkErrorPage);
        CHECKFN_MULTI("location", checkIndex);
        CHECKFN("location", checkLimitExcept);
        CHECKFN("location", checkReturn);
        CHECKFN("location", checkRoot);
        CHECKFN("location", checkUploadDir);
        if (counts[directive] > 1)
            throw runtime_error(
                Errors::Config::DirectiveNotUnique("location", directive));
        throw runtime_error(Errors::Config::UnknownDirective("location", directive));
    }
}

static inline bool addrSame(const struct in_addr &addr1, const struct in_addr &addr2) {
    return std::memcmp(&addr1, &addr2, sizeof(addr1)) == 0;
}

static bool listenDirectivesSame(const Arguments &listenDirective,
                                 const Arguments &otherListenDirective) {
    /*
    const char wildcardStr[] = "0.0.0.0";
    struct in_addr wildcard;

    if (inet_aton(wildcardStr, &wildcard) == 0)
            throw runtime_error("Logic error, 0.0.0.0 should be a valid ip address");
    */

    const string &host = listenDirective[0];
    const string &strPort = listenDirective[1];
    long port;
    struct in_addr addr;

    const string &otherHost = otherListenDirective[0];
    const string &otherStrPort = otherListenDirective[1];
    long otherPort;
    struct in_addr otherAddr;

    if (inet_aton(host.c_str(), &addr) == 0)
        throw runtime_error("Logic error, host should've been validated already");
    if (inet_aton(otherHost.c_str(), &otherAddr) == 0)
        throw runtime_error("Logic error, otherHost should've been validated already");

    port = strtol(strPort.c_str(), NULL, 10);
    otherPort = strtol(otherStrPort.c_str(), NULL, 10);

    bool hostSame = addrSame(addr, otherAddr) /* || addrSame(otherAddr, wildcard) */;
    bool portSame = port == otherPort;

    return hostSame && portSame;
}

static string overlapListen(const ArgResults &listenDirectives,
                            const ArgResults &otherListenDirectives) {
    for (ArgResults::const_iterator listenDirective = listenDirectives.begin();
         listenDirective != listenDirectives.end(); ++listenDirective) {
        for (ArgResults::const_iterator otherListenDirective =
                 otherListenDirectives.begin();
             otherListenDirective != otherListenDirectives.end();
             ++otherListenDirective) {
            if (listenDirectivesSame(*listenDirective, *otherListenDirective)) {
                return (*listenDirective)[0] + ":" + (*listenDirective)[1];
            }
        }
    }
    return "";
}

static void ensureNoOverlapServerName(const ArgResults &serverNameDirectives,
                                      const ArgResults &otherServerNameDirectives,
                                      size_t serverId, size_t otherServerId,
                                      string listenOverlap) {
    for (ArgResults::const_iterator serverNameDirective = serverNameDirectives.begin();
         serverNameDirective != serverNameDirectives.end(); ++serverNameDirective) {
        for (ArgResults::const_iterator otherServerNameDirective =
                 otherServerNameDirectives.begin();
             otherServerNameDirective != otherServerNameDirectives.end();
             ++otherServerNameDirective) {
            for (Arguments::const_iterator serverName = serverNameDirective->begin();
                 serverName != serverNameDirective->end(); ++serverName) {
                for (Arguments::const_iterator otherServerName =
                         otherServerNameDirective->begin();
                     otherServerName != otherServerNameDirective->end();
                     ++otherServerName) {
                    if (*serverName == *otherServerName) {
                        throw runtime_error(
                            "Config error: duplicate server_name's between server " +
                            repr(serverId) + " and server " + repr(otherServerId) + ": " +
                            repr(*serverName) + " (both listening on " +
                            num(listenOverlap) + ")");
                    }
                }
            }
        }
    }
}

static void ensureServerUniqueness(const Directives &directives,
                                   const ServerCtxs &servers, size_t serverId) {
    ArgResults listenDirectives = getAllDirectives(directives, "listen");
    ArgResults serverNameDirectives = getAllDirectives(directives, "server_name");

    for (ArgResults::const_iterator listenDirective = listenDirectives.begin();
         listenDirective != listenDirectives.end(); ++listenDirective) {
        for (ArgResults::const_iterator otherListenDirective = listenDirectives.begin();
             otherListenDirective != listenDirectives.end(); ++otherListenDirective) {
            if (listenDirective == otherListenDirective)
                continue;
            if (listenDirectivesSame(*listenDirective, *otherListenDirective)) {
                throw runtime_error(
                    "Config error: duplicate listen directives in server " +
                    repr(serverId) + ": " + repr(*listenDirective) + " and " +
                    repr(*otherListenDirective));
            }
        }
    }

    size_t otherServerId = 0;
    for (ServerCtxs::const_iterator server = servers.begin(); server != servers.end();
         ++server) {
        if (&directives == &server->first) {
            ++otherServerId;
            continue;
        }

        ArgResults otherListenDirectives = getAllDirectives(server->first, "listen");
        ArgResults otherServerNameDirectives =
            getAllDirectives(server->first, "server_name");

        string listenOverlap;
        if (!(listenOverlap = overlapListen(listenDirectives, otherListenDirectives))
                 .empty()) {
            ensureNoOverlapServerName(serverNameDirectives, otherServerNameDirectives,
                                      serverId, otherServerId, listenOverlap);
        }
        ++otherServerId;
    }
}

void DirectiveValidation::checkDirectives(Config &config) {
    Logger::lastInstance().trace()
        << "Validating semantics of http directives" << std::endl;
    Logger::lastInstance().trace3()
        << "http directives: " << repr(config.first) << std::endl;
    checkHttpDirectives(config.first);
    size_t serverId = 0;
    for (ServerCtxs::iterator server = config.second.begin();
         server != config.second.end(); ++server) {
        Logger::lastInstance().trace() << "Validating semantics of directives in server "
                                       << repr(serverId) << std::endl;
        Logger::lastInstance().trace3()
            << "Server " << repr(serverId) << ": " << repr(*server) << std::endl;
        checkServerDirectives(server->first);
        size_t locationId = 0;
        for (LocationCtxs::iterator location = server->second.begin();
             location != server->second.end(); ++location) {
            Logger::lastInstance().trace2()
                << "Validating semantics of directives in location block "
                << repr(locationId) << " in server " << repr(serverId) << std::endl;
            Logger::lastInstance().trace3() << "Location block " << repr(locationId)
                                            << ": " << repr(*location) << std::endl;
            checkLocationDirectives(location->second);
            ++locationId;
        }
        ++serverId;
    }
    // we have to do it in a separate loop, since in the first loop, all the listen
    // directives must be validated first
    serverId = 0;
    for (ServerCtxs::iterator server = config.second.begin();
         server != config.second.end(); ++server) {
        Logger::lastInstance().trace()
            << "Ensuring that listen & server_name directives in server "
            << repr(serverId) << " don't conflict with other servers" << std::endl;
        ensureServerUniqueness(server->first, config.second, serverId);
        ++serverId;
    }
}
