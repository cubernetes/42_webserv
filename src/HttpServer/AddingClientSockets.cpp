#include "HttpServer.hpp"
#include <fcntl.h>

void HttpServer::addClientSocketToPollFds(MultPlexFds &monitorFds, int clientSocket) {
  struct pollfd pfd;
  pfd.fd = clientSocket;
  pfd.events = POLLIN | POLLOUT;
  pfd.revents = 0;
  monitorFds.pollFds.push_back(pfd);
}

void HttpServer::addClientSocketToMonitorFds(MultPlexFds &monitorFds, int clientSocket) {
  switch (monitorFds.multPlexType) {
  case SELECT:
    throw std::logic_error("Adding select type fds not implemented yet");
    break;
  case POLL:
    addClientSocketToPollFds(monitorFds, clientSocket);
    break;
  case EPOLL:
    throw std::logic_error("Adding epoll type fds not implemented yet");
    break;
  default:
    throw std::logic_error("Adding unknown type of fd not implemented");
  }
}

void HttpServer::addNewClient(int listeningSocket) {
  struct sockaddr_in clientAddr;
  socklen_t clientLen = sizeof(clientAddr);

  int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddr, &clientLen);
  if (clientSocket < 0) {
    Logger::logError(string("accept failed: ") + strerror(errno));
    return; // don't add client on this kind of failure
  }
  if (::fcntl(clientSocket, F_SETFD, FD_CLOEXEC) < 0) {
    Logger::logError(string("fcntl failed: ") + strerror(errno));
    return; // don't add client on this kind of failure
  }

  addClientSocketToMonitorFds(_monitorFds, clientSocket);
  Logger::logDebug("New client connected. FD: " + STR(clientSocket));
}
