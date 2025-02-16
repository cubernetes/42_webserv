#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <sys/poll.h>

#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

using Constants::EPOLL;
using Constants::POLL;
using Constants::SELECT;
using std::runtime_error;

HttpServer::MultPlexFds HttpServer::getReadyPollFds(MultPlexFds &monitorFds, int nReady, struct pollfd *pollFds, nfds_t nPollFds) {
    MultPlexFds readyFds(POLL);
    vector<int> pollFdsToRemove;

    (void)nReady;
    log.trace() << "Determining readyFds from the following pollfds: " << reprArr(pollFds, nPollFds) << std::endl;
    for (size_t i = 0; i < nPollFds; ++i) {
        log.trace2() << "Value if i: " << repr(i) << std::endl;
        log.trace2() << "Checking the following pollfd: " << repr(pollFds[i]) << std::endl;
        if (pollFds[i].revents & POLLIN) {
            log.trace2() << "Poll revents contains POLLIN: " << repr(pollFds[i]) << std::endl;
            readyFds.pollFds.push_back(pollFds[i]);
            readyFds.fdStates.push_back(FD_READABLE);
        } else if (pollFds[i].revents & POLLOUT) {
            log.trace2() << "Poll revents contains POLLOUT: " << repr(pollFds[i]) << std::endl;
            readyFds.pollFds.push_back(pollFds[i]);
            readyFds.fdStates.push_back(FD_WRITEABLE);
        } else if (pollFds[i].revents != 0) {
            log.debug() << "Poll revents contains an event that is neither POLLIN nor POLLOUT: " << repr(pollFds[i]) << std::endl;
            log.debug() << "Queueing for removal" << std::endl;
            pollFdsToRemove.push_back(pollFds[i].fd);
        }
    }

    for (vector<int>::const_iterator fd = pollFdsToRemove.begin(); fd != pollFdsToRemove.end(); ++fd) {
        log.debug() << "Removing FD " << repr(*fd) << " from monitoring FDs" << std::endl;
        closeAndRemoveMultPlexFd(monitorFds, *fd);
    }
    return readyFds;
}

HttpServer::MultPlexFds HttpServer::doPoll(MultPlexFds &monitorFds) {
    struct pollfd *pollFds = multPlexFdsToPollFds(monitorFds);
    nfds_t nPollFds = getNumberOfPollFds(monitorFds);
    int nReady = ::poll(pollFds, nPollFds, Constants::multiplexTimeout);

    if (nReady < 0) {
        if (errno == EINTR) // NOTODO: @all: why is EINTR okay? What about the other codes?
                            // What about EAGAIN?
            return MultPlexFds(POLL);
        throw runtime_error(string("poll failed: ") + ::strerror(errno)); // NOTODO: @timo: make Errors::...
    }

    return getReadyPollFds(monitorFds, nReady, pollFds, nPollFds);
}

HttpServer::MultPlexFds HttpServer::determineRemoteClients(const MultPlexFds &m, vector<int> ls, const CgiFdToClientMap &cgiToClient) {
    MultPlexFds remaining;
    log.trace() << "Determining remote clients" << std::endl;
    switch (m.multPlexType) {
    case SELECT:
        throw std::logic_error("Filtering remote clients with select multiplex FDs is not implemented yet");
        break;
    case POLL:
        for (size_t i = 0; i < m.pollFds.size(); ++i) {
            int fd = m.pollFds[i].fd;
            if (std::find(ls.begin(), ls.end(), fd) == ls.end() && cgiToClient.count(fd) == 0 && _tmpCgiFds.count(fd) == 0) {
                remaining.pollFds.push_back(m.pollFds[i]);
                if (i < m.fdStates.size())
                    remaining.fdStates.push_back(m.fdStates[i]);
            }
        }
        break;
    case EPOLL:
        throw std::logic_error("Filtering remote clients with epoll multiplex FDs is not implemented yet");
        break;
    default:
        throw std::logic_error("Filtering remote clients from unknown multiplex FDs is not implemented");
    }
    log.trace() << "Remote clients are " << repr(remaining) << std::endl;
    return remaining;
}

HttpServer::MultPlexFds HttpServer::getReadyFds(MultPlexFds &monitorFds) {
    log.debug() << "Trying to get ready FDs with multiplexing method " << repr(monitorFds.multPlexType) << std::endl;
    log.trace() << "These are the monitoring FDs: " << repr(monitorFds) << std::endl;
    MultPlexFds remotes = determineRemoteClients(monitorFds, _listeningSockets, _cgiToClient);
    log.debug() << "Remote clients: " << repr(remotes) << std::endl;
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
                                   "this step"); // NOTODO: @timo: proper logging
    }
}
