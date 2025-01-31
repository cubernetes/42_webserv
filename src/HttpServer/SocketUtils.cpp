#include <algorithm>

#include "Constants.hpp"
#include "HttpServer.hpp"

using Constants::EPOLL;
using Constants::POLL;
using Constants::SELECT;

bool HttpServer::isListeningSocket(int fd) {
  return std::find(_listeningSockets.begin(), _listeningSockets.end(), fd) != _listeningSockets.end();
}

int HttpServer::multPlexFdToRawFd(const MultPlexFds &readyFds, size_t i) {
  switch (readyFds.multPlexType) {
  case SELECT:
    throw std::logic_error("Converting select fd type to raw fd not implemented");
  case POLL:
    return readyFds.pollFds[i].fd;
  case EPOLL:
    throw std::logic_error("Converting epoll fd type to raw fd not implemented");
  default:
    throw std::logic_error("Converting unknown fd type to raw fd not implemented");
  }
}

struct pollfd *HttpServer::multPlexFdsToPollFds(const MultPlexFds &fds) { return (struct pollfd *)&fds.pollFds[0]; }

nfds_t HttpServer::getNumberOfPollFds(const MultPlexFds &fds) { return static_cast<nfds_t>(fds.pollFds.size()); }
