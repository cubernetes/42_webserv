#include <ctime>

#include <signal.h>
#include <sys/wait.h>

#include "HttpServer.hpp"

static bool isTimedOut(const HttpServer::CgiProcess &process) {
  std::time_t lastActive = process.lastActive;
  std::time_t now = std::time(NULL);
  if (now - lastActive >= Constants::cgiTimeout)
    return true;
  return false;
}

void HttpServer::checkForInactiveClients() {
  vector<int> deleteFromClientToCgi;
  vector<int> deleteFromCgiToClient;
  for (ClientFdToCgiMap::iterator it = _clientToCgi.begin(); it != _clientToCgi.end(); ++it) {
    CgiProcess &process = it->second;

    if (isTimedOut(process)) {
      ::kill(process.pid, SIGKILL);
      ::waitpid(process.pid, NULL, WNOHANG);
      sendError(process.clientSocket, 504, process.location);

      closeAndRemoveMultPlexFd(_monitorFds, it->first);
      deleteFromClientToCgi.push_back(it->first);
      deleteFromCgiToClient.push_back(it->second.readFd);
    } else if (waitpid(process.pid, NULL, WNOHANG) > 0) {
      closeAndRemoveMultPlexFd(_monitorFds, it->second.readFd);
      deleteFromClientToCgi.push_back(it->first);
      deleteFromCgiToClient.push_back(it->second.readFd);
    }
  }
  for (size_t i = 0; i < deleteFromClientToCgi.size(); ++i) {
    _clientToCgi.erase(deleteFromClientToCgi[i]);
    _cgiToClient.erase(deleteFromCgiToClient[i]);
  }
}
