#include <ctime>
#include <ostream>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Repr.hpp"

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

    log.debug() << "Checking for inactive CGI clients" << std::endl;
    for (ClientFdToCgiMap::iterator it = _clientToCgi.begin(); it != _clientToCgi.end(); ++it) {
        CgiProcess &process = it->second;

        if (isTimedOut(process)) {
            log.warn() << "Timed out CGI process (sending SIGKILL): " << repr(process) << std::endl;
            (void)::kill(process.pid, SIGKILL);
            (void)::waitpid(process.pid, NULL, WNOHANG);
            sendError(process.clientSocket, 504, process.location);

            log.debug() << "Removing client " << repr(it->first) << " from clientToCgi map (CGI has no more data to send)" << std::endl;
            deleteFromClientToCgi.push_back(it->first);
            log.debug() << "Removing CGI read FD " << repr(it->second.readFd) << " (stdout) from cgiToClientMap" << std::endl;
            deleteFromCgiToClient.push_back(it->second.readFd);
            // closeAndRemoveMultPlexFd(_monitorFds, it->first);
            // _pendingWrites.erase(it->first);
        } else if (process.dead && process.noRecentReadEvent) {
            log.debug() << "Reaped CGI process: " << repr(process) << std::endl;
            closeAndRemoveMultPlexFd(_monitorFds, it->second.readFd);
            log.debug() << "Removing its read FD " << repr(it->second.readFd) << " (stdout) from cgiToClient map" << std::endl;
            deleteFromCgiToClient.push_back(it->second.readFd);
            deleteFromClientToCgi.push_back(it->first);
            // _pendingWrites.erase(it->first);
        }
        if (::waitpid(process.pid, NULL, WNOHANG) > 0)
            process.dead = true;
        process.noRecentReadEvent = true; // should be reset to false if there was a read event
    }
    for (size_t i = 0; i < deleteFromClientToCgi.size(); ++i) {
        _clientToCgi.erase(deleteFromClientToCgi[i]);
        _cgiToClient.erase(deleteFromCgiToClient[i]);
    }
}
