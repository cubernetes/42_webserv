#include <asm-generic/socket.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Config.hpp"
#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Repr.hpp"

using std::runtime_error;

static string getServerIpStr(const ServerCtx &serverCtx) {
    string listen = "listen";
    const Arguments &hostPort = getFirstDirective(serverCtx.first, listen);
    return hostPort[0];
}

static struct in_addr getServerIp(const ServerCtx &serverCtx) {
    string listen = "listen";
    const Arguments &hostPort = getFirstDirective(serverCtx.first, listen);

    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostPort[0].c_str(), NULL, &hints, &res) != 0) {
        throw runtime_error("Invalid IP address for listen directive in serverCtx config");
    }
    struct in_addr ipv4 = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    freeaddrinfo(res);
    return ipv4;
}

static in_port_t getServerPort(const ServerCtx &serverCtx) {
    string listen = "listen";
    const Arguments &hostPort = getFirstDirective(serverCtx.first, listen);
    return htons((in_port_t)std::atoi(hostPort[1].c_str()));
}

static int createTcpListenSocket() {
    int listeningSocket = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (listeningSocket < 0)
        throw runtime_error("Failed to create socket");

    int opt = 1;
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        ::close(listeningSocket);
        throw runtime_error("Failed to set socket options");
    }
    return listeningSocket;
}

static void bindSocket(int listeningSocket, const HttpServer::Server &server) {
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = server.port;
    address.sin_addr = server.ip;

    if (bind(listeningSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        ::close(listeningSocket);
        throw runtime_error(string("bind error: ") + strerror(errno));
    }
}

static void listenSocket(int listeningSocket) {
    if (listen(listeningSocket, SOMAXCONN) < 0) {
        ::close(listeningSocket);
        throw runtime_error(string("listen error: ") + strerror(errno));
    }
}

static bool alreadyListening(const HttpServer::Server &server, const HttpServer::Servers &servers) {
    for (HttpServer::Servers::const_iterator otherServer = servers.begin(); otherServer != servers.end();
         ++otherServer) {
        struct in_addr otherAddr = otherServer->ip;
        in_port_t otherPort = otherServer->port;
        if (server.port == otherPort &&
            (::memcmp(&otherAddr, &server.ip, sizeof(otherAddr)) == 0 || otherAddr.s_addr == INADDR_ANY))
            return true;
    }
    return false;
}

void HttpServer::setupListeningSocket(const Server &server) {
    if (alreadyListening(server, _servers))
        return;
    int listeningSocket = createTcpListenSocket();

    bindSocket(listeningSocket, server);
    listenSocket(listeningSocket);

    struct pollfd pfd;
    pfd.fd = listeningSocket;
    pfd.events = POLLIN;
    _monitorFds.pollFds.push_back(pfd);

    _listeningSockets.push_back(listeningSocket);
}

void HttpServer::setupServers(const Config &config) {
    for (ServerCtxs::const_iterator serverCtx = config.second.begin(); serverCtx != config.second.end(); ++serverCtx) {
        Server server(serverCtx->first, serverCtx->second, getFirstDirective(serverCtx->first, "server_name"));

        server.ip = getServerIp(*serverCtx);
        server.port = getServerPort(*serverCtx);

        setupListeningSocket(server);
        log.info() << cmt("Server with names ") << repr(server.serverNames) << cmt(" is listening on ")
                   << num(getServerIpStr(*serverCtx)) << num(":") << repr(ntohs(server.port)) << '\n';

        _servers.push_back(server);

        AddrPort addr(server.ip, server.port);
        if (_defaultServers.find(addr) == _defaultServers.end())
            _defaultServers[addr] = _servers.size() - 1;
    }
}
