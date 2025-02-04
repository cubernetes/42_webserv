#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <netinet/in.h>
#include <ostream>
#include <stdexcept>
#include <sys/socket.h>

#include "Config.hpp"
#include "HttpServer.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

using std::runtime_error;
using Utils::STR;

size_t HttpServer::getIndexOfServerByHost(const string &requestedHost,
                                          const struct in_addr &addr,
                                          in_port_t port) const {
    string requestedHostLower = Utils::strToLower(requestedHost);
    for (size_t i = 0; i < _servers.size(); ++i) {
        const Server &server = _servers[i];
        if (server.port == port && (memcmp(&server.ip, &addr, sizeof(addr)) == 0 ||
                                    server.ip.s_addr == INADDR_ANY)) {
            for (size_t j = 0; j < server.serverNames.size(); ++j) {
                if (requestedHostLower ==
                    Utils::strToLower(
                        server.serverNames
                            [j])) { // see
                                    // https://datatracker.ietf.org/doc/html/rfc2616#section-3.2.3
                    return i + 1;
                }
            }
        }
    }
    return 0;
}

size_t HttpServer::getIndexOfDefaultServer(const struct in_addr &addr,
                                           in_port_t port) const {
    AddrPort addrPort(addr, port);
    DefaultServers::const_iterator exactMatch = _defaultServers.find(addrPort);
    if (exactMatch != _defaultServers.end()) {
        return exactMatch->second + 1;
    }

    struct in_addr addrDefault;
    addrDefault.s_addr = INADDR_ANY;
    AddrPort addrPortDefault(addrDefault, port);
    DefaultServers::const_iterator anyMatch = _defaultServers.find(addrPortDefault);
    if (anyMatch != _defaultServers.end()) {
        return anyMatch->second + 1;
    }
    return 0;
}

size_t HttpServer::findMatchingServer(const string &host, const struct in_addr &addr,
                                      in_port_t port) const {
    string requestedHost = host;
    size_t colonPos = requestedHost.rfind(':');
    if (colonPos != string::npos)
        requestedHost = requestedHost.substr(0, colonPos);

    size_t idx = getIndexOfServerByHost(requestedHost, addr, port);
    if (idx)
        return idx;

    return getIndexOfDefaultServer(addr, port);
}

size_t HttpServer::findMatchingLocation(const Server &server, const string &path) const {
    int bestIdx = -1;
    int bestScore = -1;
    log.debug() << "For path " << path
                << ": Trying to find a location in this server: " << repr(server)
                << std::endl;
    for (size_t i = 0; i < server.locations.size(); ++i) {
        LocationCtx loc = server.locations[i];
        const string &locPath = loc.first;
        if (Utils::isPrefix(locPath, path)) {
            log.debug() << "Match: Location path " << locPath << " is a prefix of path "
                        << path << std::endl;
            pair<string::const_iterator, string::const_iterator> matcher =
                std::mismatch(locPath.begin(), locPath.end(), path.begin());
            int currentScore =
                static_cast<int>(std::distance(locPath.begin(), matcher.first));
            if (currentScore > bestScore) {
                bestScore = currentScore;
                bestIdx = static_cast<int>(i);
            }
        } else {
            log.debug() << "Location path " << locPath << " was not a prefix of path "
                        << path << std::endl;
        }
    }
    return static_cast<size_t>(
        bestIdx + 1); // will be 0 when nothing was found. a default / location block is
                      // always added at the end of the location vector that SHOULD always
                      // match (every path must start with /)
}

string getHost(const HttpServer::HttpRequest &request) {
    string host;
    if (request.headers.find("Host") != request.headers.end()) {
        if (request.headers.find("Host") != request.headers.end()) {
            host = request.headers.at("Host");
            size_t colonPos = host.find(':');
            if (colonPos != string::npos)
                (void)host.substr(0, colonPos);
        }
    }
    return host; // empty string should match default server_name
}

struct sockaddr_in HttpServer::getSockaddrIn(int clientSocket) {
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    if (getsockname(clientSocket, (struct sockaddr *)&addr, &addrLen) < 0) {
        int prev_errno = errno;
        sendError(clientSocket, 500, NULL);
        throw runtime_error(string("getsockname failed: ") + strerror(prev_errno));
    }
    return addr;
}

const LocationCtx &HttpServer::requestToLocation(int clientSocket,
                                                 const HttpRequest &request) {
    struct sockaddr_in addr = getSockaddrIn(clientSocket);

    // Get host from request headers
    string host = getHost(request);

    // find server by matching against host, addr, and port
    size_t serverIdx;
    if (!(serverIdx = findMatchingServer(host, addr.sin_addr, addr.sin_port))) {
        sendError(clientSocket, 404, NULL); // TODO: @all: is 404 rlly correct?
        throw runtime_error(string("Couldn't find a server for hostname '") + host +
                            "' and addr:port being " +
                            repr<struct sockaddr_in_wrapper>(addr));
    }
    const Server &server =
        _servers[serverIdx -
                 1]; // serverIdx=0 indicates failure, so 1 is the first server

    // find location by matching against request uri
    size_t locationIdx;
    if (!(locationIdx = findMatchingLocation(server, request.path))) {
        sendError(clientSocket, 404, NULL); // TODO: @all: is 404 rlly correct?
        throw runtime_error(string("Couldn't find a location for URI '") + request.path +
                            "'");
    }
    return server.locations[locationIdx - 1];
}
