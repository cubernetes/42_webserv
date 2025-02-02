#include <cstddef>
#include <stdexcept>
#include <sys/poll.h>

#include "Constants.hpp"
#include "HttpServer.hpp"

using Constants::EPOLL;
using Constants::POLL;
using Constants::SELECT;

void HttpServer::updatePollEvents(MultPlexFds &monitorFds, int clientSocket, short events, bool add) {
    for (size_t i = 0; i < monitorFds.pollFds.size(); ++i) {
        if (monitorFds.pollFds[i].fd == clientSocket) {
            if (add)
                monitorFds.pollFds[i].events |= events;
            else
                monitorFds.pollFds[i].events &= ~events;
            break;
        }
    }
}

void HttpServer::startMonitoringForWriteEvents(MultPlexFds &monitorFds, int clientSocket) {
    switch (monitorFds.multPlexType) {
    case SELECT:
        throw std::logic_error("Starting monitoring for write events for select type fds not implemented yet");
        break;
    case POLL:
        updatePollEvents(monitorFds, clientSocket, POLLOUT, true);
        break;
    case EPOLL:
        throw std::logic_error("Starting monitoring for write events for epoll type fds not implemented yet");
        break;
    default:
        throw std::logic_error("Starting monitoring for write events for unknown type fds not implemented yet");
    }
}

void HttpServer::stopMonitoringForWriteEvents(MultPlexFds &monitorFds, int clientSocket) {
    switch (monitorFds.multPlexType) {
    case SELECT:
        throw std::logic_error("Stopping monitoring for write events for select type fds not implemented yet");
        break;
    case POLL:
        updatePollEvents(monitorFds, clientSocket, POLLOUT, false);
        break;
    case EPOLL:
        throw std::logic_error("Stopping monitoring for write events for epoll type fds not implemented yet");
        break;
    default:
        throw std::logic_error("Stopping monitoring for write events for unknown type fds not implemented yet");
    }
}
