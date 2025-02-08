#include <algorithm>
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <sys/poll.h>

#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Repr.hpp"

using Constants::EPOLL;
using Constants::POLL;
using Constants::SELECT;

bool HttpServer::isListeningSocket(int fd) {
    bool isListening = std::find(_listeningSockets.begin(), _listeningSockets.end(),
                                 fd) != _listeningSockets.end();
    log.trace() << "All listening sockets: " << repr(_listeningSockets) << std::endl;
    log.trace() << "Checking if " << repr(fd)
                << " is a listening socket: " << repr(isListening)
                << std::endl; // TODO: @timo: overload << s.t. you don't have to do
                              // repr(intVar) all the time
    return isListening;
}

vector<int> HttpServer::multPlexFdsToRawFds(const MultPlexFds &readyFds) {
    vector<int> rawFds;
    switch (readyFds.multPlexType) {
    case SELECT:
        throw std::logic_error("Converting select fd type to raw fd not implemented");
    case POLL:
        log.trace() << "Converting pollfds " << repr(readyFds.pollFds) << " to plain FDs"
                    << std::endl;
        for (size_t i = 0; i < readyFds.pollFds.size(); ++i)
            rawFds.push_back(readyFds.pollFds[i].fd);
        return rawFds;
    case EPOLL:
        throw std::logic_error("Converting epoll fd type to raw fd not implemented");
    default:
        throw std::logic_error("Converting unknown fd type to raw fd not implemented");
    }
}

int HttpServer::multPlexFdToRawFd(const MultPlexFds &readyFds, size_t i) {
    switch (readyFds.multPlexType) {
    case SELECT:
        throw std::logic_error("Converting select fd type to raw fd not implemented");
    case POLL:
        log.trace() << "Converting pollfd " << repr(readyFds.pollFds[i])
                    << " to plain FD " << repr(readyFds.pollFds[i].fd) << std::endl;
        return readyFds.pollFds[i].fd;
    case EPOLL:
        throw std::logic_error("Converting epoll fd type to raw fd not implemented");
    default:
        throw std::logic_error("Converting unknown fd type to raw fd not implemented");
    }
}

struct pollfd *HttpServer::multPlexFdsToPollFds(const MultPlexFds &fds) {
    return (struct pollfd *)&fds.pollFds[0];
}

nfds_t HttpServer::getNumberOfPollFds(const MultPlexFds &fds) {
    return static_cast<nfds_t>(fds.pollFds.size());
}
