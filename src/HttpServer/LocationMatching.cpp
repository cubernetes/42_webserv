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
#include "Logger.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

using std::runtime_error;

size_t HttpServer::getIndexOfServerByHost(const string &requestedHost, const struct in_addr &addr, in_port_t port) const {
    string requestedHostLower = Utils::strToLower(requestedHost);
    for (size_t i = 0; i < _servers.size(); ++i) {
        const Server &server = _servers[i];
        if (server.port == port && (std::memcmp(&server.ip, &addr, sizeof(addr)) == 0 || server.ip.s_addr == INADDR_ANY)) {
            for (size_t j = 0; j < server.serverNames.size(); ++j) {
                if (requestedHostLower ==
                    Utils::strToLower(server.serverNames[j])) { // see
                                                                // https://datatracker.ietf.org/doc/html/rfc2616#section-3.2.3
                    return i + 1;
                }
            }
        }
    }
    return 0;
}

size_t HttpServer::getIndexOfDefaultServer(const struct in_addr &addr, in_port_t port) const {
    log.debug() << "Searching for a default server for addr:port " << repr(sockaddr_in_wrapper(addr, port)) << std::endl;
    AddrPort addrPort(addr, port);
    DefaultServers::const_iterator exactMatch = _defaultServers.find(addrPort);
    if (exactMatch != _defaultServers.end()) {
        log.debug() << "Found default server with id " << repr(exactMatch->second) << std::endl;
        return exactMatch->second + 1;
    }

    log.debug() << "Couldn't find default server using exact addr:port match, trying "
                   "INADDR_ANY/0.0.0.0"
                << std::endl;
    struct in_addr addrDefault;
    addrDefault.s_addr = INADDR_ANY;
    AddrPort addrPortDefault(addrDefault, port);
    DefaultServers::const_iterator anyMatch = _defaultServers.find(addrPortDefault);
    if (anyMatch != _defaultServers.end()) {
        log.debug() << "Found default server (using INADDR_ANY/0.0.0.0) with id " << repr(exactMatch->second) << std::endl;
        return anyMatch->second + 1;
    }
    log.debug() << "Could not find any default server for addr:port " << repr(sockaddr_in_wrapper(addr, port)) << std::endl;
    return 0;
}

size_t HttpServer::findMatchingServer(const string &host, const struct in_addr &addr, in_port_t port) const {
    log.debug() << "Trying to find matching server for host " << repr(host) << " and addr:port " << repr(sockaddr_in_wrapper(addr, port))
                << std::endl;
    string requestedHost = host;
    size_t colonPos = requestedHost.rfind(':');
    if (colonPos != string::npos)
        requestedHost = requestedHost.substr(0, colonPos);

    size_t idx = getIndexOfServerByHost(requestedHost, addr, port);
    if (idx) {
        log.debug() << "Found direct match for server (id) " << repr(idx - 1) << std::endl;
        log.trace() << "Server is " << repr(_servers[idx - 1]) << std::endl;
        return idx;
    }

    log.debug() << "Couldn't match host:addr:port directly, trying to find a default server" << std::endl;
    return getIndexOfDefaultServer(addr, port);
}

size_t HttpServer::findMatchingLocation(const Server &server, const string &path) const {
    int bestIdx = -1;
    int bestScore = -1;
    log.debug() << "For path " << repr(path) << ": Trying to find a location block in this server: " << repr(server) << std::endl;
    for (size_t i = 0; i < server.locations.size(); ++i) {
        LocationCtx loc = server.locations[i];
        const string &locPath = loc.first;
        if (Utils::isPrefix(locPath, path)) {
            log.debug() << "Match: Location path " << repr(locPath) << " is a prefix of path " << repr(path) << std::endl;
            pair<string::const_iterator, string::const_iterator> matcher = std::mismatch(locPath.begin(), locPath.end(), path.begin());
            int currentScore = static_cast<int>(std::distance(locPath.begin(), matcher.first));
            log.debug() << "Prefix length is " << repr(currentScore) << " and highest prefix length is " << repr(bestScore) << std::endl;
            if (currentScore > bestScore) {
                bestScore = currentScore;
                bestIdx = static_cast<int>(i);
                log.debug() << "Updating highest prefix length to " << repr(currentScore) << " and index to " << repr(bestIdx) << std::endl;
            }
        } else {
            log.debug() << "Location path " << repr(locPath) << " was not a prefix of path " << repr(path) << std::endl;
        }
    }
    if (bestIdx == -1)
        log.error() << "Could not found a location block for path " << repr(path) << ", this should never happen" << std::endl;
    else
        log.debug() << "Found a location for path " << repr(path) << ", index of location block is " << repr(bestIdx) << std::endl;
    return static_cast<size_t>(bestIdx + 1); // will be 0 when nothing was found. a default / location block is
                                             // always added at the end of the location vector that SHOULD always
                                             // match (every path must start with /)
}

string getHost(const HttpServer::HttpRequest &request) {
    Logger::lastInstance().debug() << "Getting host value from request " << repr(request) << std::endl;
    string host;
    bool notFound = true;
    if (request.headers.find("host") != request.headers.end()) {
        if (request.headers.find("host") != request.headers.end()) {
            host = request.headers.at("host");
            notFound = false;
            size_t colonPos = host.find(':');
            if (colonPos != string::npos)
                host = host.substr(0, colonPos);
        }
    }
    if (notFound)
        Logger::lastInstance().debug() << "Host header not found in request, leaving empty" << std::endl;
    Logger::lastInstance().debug() << "Returning host " << repr(host) << std::endl;
    return host; // empty string should match default server_name
}

struct sockaddr_in HttpServer::getSockaddrIn(int clientSocket) {
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    log.debug() << "Calling " << func("getsockname") << punct("()") << " on socket " << repr(clientSocket) << std::endl;
    if (::getsockname(clientSocket, reinterpret_cast<struct sockaddr *>(&addr), &addrLen) < 0) {
        int prev_errno = errno;
        sendError(clientSocket, 500, NULL);
        throw runtime_error("Error calling " + func("getsockname") + punct("()") + ": " + ::strerror(prev_errno));
    }
    return addr;
}

const LocationCtx &HttpServer::requestToLocation(int clientSocket, const HttpRequest &request) {
    log.debug() << "Converting socket " << repr(clientSocket) << " to sockaddr_in" << std::endl;
    struct sockaddr_in addr = getSockaddrIn(clientSocket);

    // Get host from request headers
    string host = getHost(request);

    // find server by matching against host, addr, and port
    size_t serverIdx;
    if (!(serverIdx = findMatchingServer(host, addr.sin_addr, addr.sin_port))) {
        sendError(clientSocket, 404, NULL);
        throw runtime_error("Couldn't find any server for hostname '" + repr(host) + "' and addr:port being " +
                            repr<struct sockaddr_in_wrapper>(addr));
    }
    const Server &server = _servers[serverIdx - 1]; // serverIdx=0 indicates failure, so 1 is the first server

    // find location by matching against request path
    size_t locationIdx;
    if (!(locationIdx = findMatchingLocation(server, request.path))) {
        sendError(clientSocket, 404, NULL);
        throw runtime_error(string("Could not find a location for path ") + repr(request.path));
    }
    log.debug() << "Request " << repr(request) << " was successfully translated to location " << repr(server.locations[locationIdx - 1])
                << std::endl;
    return server.locations[locationIdx - 1];
}
