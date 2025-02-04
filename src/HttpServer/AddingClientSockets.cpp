#include <cerrno>

#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <ostream>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

using Constants::EPOLL;
using Constants::POLL;
using Constants::SELECT;

void HttpServer::addClientSocketToPollFds(MultPlexFds &monitorFds, int clientSocket) {
    struct pollfd pfd;
    pfd.fd = clientSocket;
    pfd.events = POLLIN | POLLOUT;
    pfd.revents = 0;
    log.debug() << "Adding pollfd " << pfd << " to monitoring FDs" << std::endl;
    monitorFds.pollFds.push_back(pfd);
}

void HttpServer::addClientSocketToMonitorFds(MultPlexFds &monitorFds, int clientSocket) {
    switch (monitorFds.multPlexType) {
    case SELECT:
        throw std::logic_error("Adding select type FDs not implemented yet");
        break;
    case POLL:
        addClientSocketToPollFds(monitorFds, clientSocket);
        break;
    case EPOLL:
        throw std::logic_error("Adding epoll type FDs not implemented yet");
        break;
    default:
        throw std::logic_error("Adding unknown type of fd not implemented");
    }
}

void HttpServer::addNewClient(int listeningSocket) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    log.debug() << "Calling accept() on socket " << listeningSocket << std::endl;
    int clientSocket =
        accept(listeningSocket, (struct sockaddr *)&clientAddr, &clientLen);
    log.debug() << "Got client socket FD back: " << clientSocket << std::endl;
    if (clientSocket < 0) {
        log.error() << "accept() failed: " << strerror(errno) << std::endl;
        return; // don't add client on this kind of failure
    }

    addClientSocketToMonitorFds(_monitorFds, clientSocket);
    log.info() << "New client connected with FD " << repr(clientSocket)
               << " and addr:port " << repr<struct sockaddr_in_wrapper>(clientAddr)
               << std::endl;
}
