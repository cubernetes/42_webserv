#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <unistd.h>

#include "Ansi.hpp"
#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Repr.hpp"

using Constants::EPOLL;
using Constants::POLL;
using Constants::SELECT;

void HttpServer::removePollFd(MultPlexFds &monitorFds, int fd) {
    for (PollFds::iterator pollFd = monitorFds.pollFds.begin(); pollFd != monitorFds.pollFds.end(); ++pollFd) {
        if (pollFd->fd == fd) {
            monitorFds.pollFds.erase(pollFd);
            break;
        }
    }
}

void HttpServer::closeAndRemoveAllPollFd(MultPlexFds &monitorFds) {
    for (PollFds::iterator pollFd = monitorFds.pollFds.begin(); pollFd != monitorFds.pollFds.end(); ++pollFd) {
        if (pollFd->fd >= 0) {
            ::close(pollFd->fd);
            pollFd->fd = -1;
        }
    }
    monitorFds.pollFds.clear();
}

void HttpServer::closeAndRemoveMultPlexFd(MultPlexFds &monitorFds, int fd) {
    vector<int> rawClientFds = multPlexFdsToRawFds(determineRemoteClients(_monitorFds, _listeningSockets, _cgiToClient));
    bool isClientSocket = std::find(rawClientFds.begin(), rawClientFds.end(), fd) != rawClientFds.end();
    log.trace() << "Is FD " << repr(fd) << " a remote client FD?: " << repr(isClientSocket) << std::endl;

    if (persistConns.count(fd) > 0) {
        log.debug() << "Connection on FD " << repr(fd) << " has handled a response, but is persistent, therefore not closing" << std::endl;
        return;
    }

    Logger::StreamWrapper &out = isClientSocket ? log.info : log.debug;

    out();

    if (!ansi::noColor() && isClientSocket)
        out << ANSI_YELLOW_BG ANSI_BLACK;
    Constants::forceNoColor = true;

    out << "Calling close() on FD " << repr(fd);

    Constants::forceNoColor = false;
    if (!ansi::noColor() && isClientSocket)
        out << ANSI_RST;

    out << std::endl;

    ::close(fd); // TODO: @timo: guard every syscall
    _tmpCgiFds.erase(fd);
    log.debug() << "Removing FD " << repr(fd) << " from monitoring FDs" << std::endl;
    switch (monitorFds.multPlexType) {
    case SELECT:
        throw std::logic_error("Removing select type FDs not implemented yet");
        break;
    case POLL:
        removePollFd(monitorFds, fd);
        break;
    case EPOLL:
        throw std::logic_error("Removing epoll type FDs not implemented yet");
        break;
    default:
        throw std::logic_error("Removing unknown type of fd not implemented");
    }
}

void HttpServer::closeAndRemoveAllMultPlexFd(MultPlexFds &monitorFds) {
    switch (monitorFds.multPlexType) {
    case SELECT:
        throw std::logic_error("Removing all select type FDs not implemented yet");
        break;
    case POLL:
        closeAndRemoveAllPollFd(monitorFds);
        break;
    case EPOLL:
        throw std::logic_error("Removing all epoll type FDs not implemented yet");
        break;
    default:
        throw std::logic_error("Removing all unknown types of fd not implemented");
    }
}

void HttpServer::removeClient(int clientSocket) { closeAndRemoveMultPlexFd(_monitorFds, clientSocket); }
