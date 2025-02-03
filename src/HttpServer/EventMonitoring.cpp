#include <cerrno>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <sys/poll.h>

#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Repr.hpp"

using Constants::EPOLL;
using Constants::POLL;
using Constants::SELECT;
using std::runtime_error;

HttpServer::MultPlexFds HttpServer::getReadyPollFds(MultPlexFds &monitorFds, int nReady,
                                                    struct pollfd *pollFds,
                                                    nfds_t nPollFds) {
    MultPlexFds readyFds(POLL);

    (void)nReady;
    for (int i = 0; i < static_cast<int>(nPollFds); ++i) {
        if (pollFds[i].revents & POLLIN) {
            readyFds.pollFds.push_back(pollFds[i]);
            readyFds.fdStates.push_back(FD_READABLE);
        } else if (pollFds[i].revents & POLLOUT) {
            readyFds.pollFds.push_back(pollFds[i]);
            readyFds.fdStates.push_back(FD_WRITEABLE);
        } else if (pollFds[i].revents != 0) {
            closeAndRemoveMultPlexFd(monitorFds, pollFds[i].fd);
        }
    }

    return readyFds;
}

HttpServer::MultPlexFds HttpServer::doPoll(MultPlexFds &monitorFds) {
    struct pollfd *pollFds = multPlexFdsToPollFds(monitorFds);
    nfds_t nPollFds = getNumberOfPollFds(monitorFds);
    int nReady = poll(pollFds, nPollFds, Constants::multiplexTimeout);

    if (nReady < 0) {
        if (errno == EINTR) // TODO: @all: why is EINTR okay? What about the other codes?
                            // What about EAGAIN?
            return MultPlexFds(POLL);
        throw runtime_error(string("poll failed: ") +
                            strerror(errno)); // TODO: @timo: make Errors::...
    }

    return getReadyPollFds(monitorFds, nReady, pollFds, nPollFds);
}

HttpServer::MultPlexFds HttpServer::getReadyFds(MultPlexFds &monitorFds) {
    log.debug() << "Trying to get ready FDs with multiplexing method '"
                << repr(monitorFds.multPlexType) << "'" << std::endl;
    switch (monitorFds.multPlexType) {
    case SELECT:
        throw std::logic_error("Getting ready FDs from select not implemented yet");
        break;
    case POLL:
        return doPoll(monitorFds);
        break;
    case EPOLL:
        throw std::logic_error("Getting ready FDs from epoll not implemented yet");
        break;
    default:
        throw std::logic_error("Getting ready FDs from unknown method not implemented");
    }
}

void HttpServer::handleReadyFds(const MultPlexFds &readyFds) {
    size_t nReadyFds = readyFds.fdStates.size();
    log.debug() << "Number of ready FDs: " << repr(nReadyFds) << std::endl;
    if (nReadyFds == 0)
        return;
    log.debug() << "Handling the following ready FDs: " << repr(readyFds) << std::endl;
    for (size_t i = 0; i < nReadyFds; ++i) {
        int fd = multPlexFdToRawFd(readyFds, i);
        if (readyFds.fdStates[i] == FD_READABLE) {
            if (isListeningSocket(fd))
                addNewClient(fd);
            else
                readFromClient(fd);
        } else if (readyFds.fdStates[i] == FD_WRITEABLE)
            writeToClient(fd);
        else
            throw std::logic_error("Cannot handle fd other than readable or writable at "
                                   "this step"); // TODO: @timo: proper logging
    }
}
