#include <asm-generic/socket.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <ostream>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Config.hpp"
#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

using std::runtime_error;

static struct in_addr getServerIp(const ServerCtx &serverCtx) {
    Logger::lastInstance().debug()
        << "Trying to get server IP from listen directive" << std::endl;
    string listen = "listen";
    const Arguments &hostPort = getFirstDirective(serverCtx.first, listen);

    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    Logger::lastInstance().debug()
        << "Calling " << func("getaddrinfo") << punct("()") << std::endl;
    if (::getaddrinfo(hostPort[0].c_str(), NULL, &hints, &res) != 0)
        throw runtime_error(
            "Invalid IP address for listen directive in serverCtx config");
    struct in_addr ipv4 = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ::freeaddrinfo(res);
    Logger::lastInstance().debug()
        << "Successfully got IPv4 address: " << repr(ipv4) << std::endl;
    return ipv4;
}

static in_port_t getServerPort(const ServerCtx &serverCtx) {
    Logger::lastInstance().debug()
        << "Trying to get server port from listen directive" << std::endl;
    string listen = "listen";
    const Arguments &hostPort = getFirstDirective(serverCtx.first, listen);
    in_port_t port = ::htons((in_port_t)std::atoi(hostPort[1].c_str()));
    Logger::lastInstance().debug()
        << "Successfully got port: " << repr(::ntohs(port)) << std::endl;
    return port;
}

static int createTcpListenSocket() {
    Logger::lastInstance().debug()
        << "Creating AF_INET SOCK_STREAM socket (TCP)" << std::endl;
    int listeningSocket =
        ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC,
                 0); // TODO: @timo: close all fds from _monitorFds array
                     // // TODO: @timo: close all fds from _monitorFds and
                     // associated CGI read/write fds whenever forking.
    if (listeningSocket < 0)
        throw runtime_error(string("Failed to create socket: ") + ::strerror(errno));
    Logger::lastInstance().debug()
        << "Successfully created socket with file descriptor number "
        << repr(listeningSocket) << std::endl;

    int opt = 1;
    Logger::lastInstance().debug() << "Setting socket " << repr(listeningSocket)
                                   << " options SO_REUSEADDR" << std::endl;
    if (::setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        ::close(listeningSocket);
        throw runtime_error(string("Failed to set socket options") + ::strerror(errno));
    }
    return listeningSocket;
}

static void bindSocket(int listeningSocket, const HttpServer::Server &server) {
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = server.port;
    address.sin_addr = server.ip;

    Logger::lastInstance().debug()
        << "Binding socket " << repr(listeningSocket) << " to "
        << repr(sockaddr_in_wrapper(server.ip, server.port)) << std::endl;
    if (::bind(listeningSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        ::close(listeningSocket);
        throw runtime_error(string("bind error: ") + ::strerror(errno));
    }
}

static void listenSocket(int listeningSocket) {
    Logger::lastInstance().debug()
        << "Starting listening on socket " << repr(listeningSocket) << std::endl;
    if (::listen(listeningSocket, SOMAXCONN) < 0) {
        ::close(listeningSocket);
        throw runtime_error(string("listen error: ") + ::strerror(errno));
    }
}

static bool alreadyListening(const HttpServer::Server &server,
                             const HttpServer::Servers &servers) {
    for (HttpServer::Servers::const_iterator otherServer = servers.begin();
         otherServer != servers.end(); ++otherServer) {
        struct in_addr otherAddr = otherServer->ip;
        in_port_t otherPort = otherServer->port;
        if (server.port == otherPort &&
            (std::memcmp(&otherAddr, &server.ip, sizeof(otherAddr)) == 0 ||
             otherAddr.s_addr == INADDR_ANY)) {
            Logger::lastInstance().debug()
                << "Already listening on addr:port: "
                << repr(sockaddr_in_wrapper(otherAddr, otherPort)) << ", skipping"
                << std::endl;
            return true;
        }
    }
    return false;
}

void HttpServer::setupListeningSocket(const Server &server) {
    if (alreadyListening(server, _servers))
        return;
    int listeningSocket = createTcpListenSocket();

    bindSocket(listeningSocket, server);
    listenSocket(listeningSocket);

    Logger::lastInstance().debug() << "Creating a pollfd with POLLIN events for socket "
                                   << repr(listeningSocket) << std::endl;
    struct pollfd pfd;
    pfd.fd = listeningSocket;
    pfd.events = POLLIN;
    _monitorFds.pollFds.push_back(pfd);

    _listeningSockets.push_back(listeningSocket);
}

void HttpServer::setupServers(const Config &config) {
    log.debug() << "Setting up servers" << std::endl;
    size_t serverId = 0;
    for (ServerCtxs::const_iterator serverCtx = config.second.begin();
         serverCtx != config.second.end(); ++serverCtx) {
        Server server(serverCtx->first, serverCtx->second,
                      getFirstDirective(serverCtx->first, "server_name"), log, serverId);

        server.ip = getServerIp(*serverCtx);
        server.port = getServerPort(*serverCtx);

        setupListeningSocket(server);
        log.info() << "Server with names " << repr(server.serverNames)
                   << " is listening on "
                   << repr(sockaddr_in_wrapper(server.ip, server.port)) << '\n';

        _servers.push_back(server);

        AddrPort addr(server.ip, server.port);
        if (_defaultServers.find(addr) == _defaultServers.end()) {
            Logger::lastInstance().debug()
                << "Setting up the default server for addr:port "
                << repr(sockaddr_in_wrapper(server.ip, server.port))
                << " to the server with server names "
                << repr(getFirstDirective(_servers.back().directives, "server_name"))
                << std::endl;
            _defaultServers[addr] = _servers.size() - 1;
        }
        ++serverId;
    }
}
