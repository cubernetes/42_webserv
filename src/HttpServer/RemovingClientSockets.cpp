#include <stdexcept>
#include <unistd.h>

#include "Constants.hpp"
#include "HttpServer.hpp"

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
    ::close(fd); // TODO: @timo: guard every syscall
    switch (monitorFds.multPlexType) {
    case SELECT:
        throw std::logic_error("Removing select type fds not implemented yet");
        break;
    case POLL:
        removePollFd(monitorFds, fd);
        break;
    case EPOLL:
        throw std::logic_error("Removing epoll type fds not implemented yet");
        break;
    default:
        throw std::logic_error("Removing unknown type of fd not implemented");
    }
}

void HttpServer::closeAndRemoveAllMultPlexFd(MultPlexFds &monitorFds) {
    switch (monitorFds.multPlexType) {
    case SELECT:
        throw std::logic_error("Removing all select type fds not implemented yet");
        break;
    case POLL:
        closeAndRemoveAllPollFd(monitorFds);
        break;
    case EPOLL:
        throw std::logic_error("Removing all epoll type fds not implemented yet");
        break;
    default:
        throw std::logic_error("Removing all unknown types of fd not implemented");
    }
}

void HttpServer::removeClient(int clientSocket) {
    // TODO: @all: this function should actually be the one that removes all PendingWrites, PendingCloses,
    // PendingRequests, cgi stuff
    closeAndRemoveMultPlexFd(_monitorFds, clientSocket);
}
