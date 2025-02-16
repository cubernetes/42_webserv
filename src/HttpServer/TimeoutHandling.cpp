#include <ctime>
#include <ostream>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

static bool isTimedOut(const HttpServer::CgiProcess &process) {
    std::time_t lastActive = process.lastActive;
    std::time_t now = std::time(NULL);
    Logger::lastInstance().debug() << "Checking wether CGI process " << repr(process) << " is timed out" << std::endl;
    Logger::lastInstance().debug() << "Now: " << repr(Utils::formattedTimestamp(now)) << " (" << repr(now) << ")" << std::endl;
    Logger::lastInstance().debug() << "Last active: " << repr(Utils::formattedTimestamp(lastActive)) << " (" << repr(lastActive) << ")"
                                   << std::endl;
    Logger::lastInstance().debug() << "Difference: " << repr(now - lastActive) << std::endl;
    Logger::lastInstance().debug() << "Timeout: " << repr(Constants::cgiTimeout) << std::endl;
    if (now - lastActive >= Constants::cgiTimeout) {
        Logger::lastInstance().debug() << "CGI process IS timed out (difference is bigger)" << std::endl;
        return true;
    }
    Logger::lastInstance().debug() << "CGI process is NOT timed out (difference is smaller)" << std::endl;
    return false;
}

void HttpServer::checkForInactiveClients() {
    vector<int> deleteFromClientToCgi;
    vector<int> deleteFromCgiToClient;

    log.debug() << "Checking for inactive CGI clients" << std::endl;
    for (ClientFdToCgiMap::iterator it = _clientToCgi.begin(); it != _clientToCgi.end(); ++it) {
        int clientSocket = it->first;
        CgiProcess &process = it->second;

        if (process.done) {
            log.trace() << "Process is marked as done, not checking it: " << repr(process) << std::endl;
            continue;
        }

        if (isTimedOut(process)) {
            log.warning() << "Timed out CGI process (sending SIGKILL): " << repr(process) << std::endl;
            (void)::kill(process.pid, SIGKILL);
            (void)::waitpid(process.pid, NULL, WNOHANG);
            _pendingCloses.erase(process.clientSocket); // overwrite behaviour
            sendError(process.clientSocket, 504, process.location);

            log.debug() << "Removing client " << repr(clientSocket) << " from clientToCgi map (CGI has no more data to send)" << std::endl;
            deleteFromClientToCgi.push_back(clientSocket);
            log.debug() << "Removing CGI read FD " << repr(process.readFd) << " (stdout) from cgiToClientMap" << std::endl;
            deleteFromCgiToClient.push_back(process.readFd);
            log.debug() << "Inserting CGI read FD " << repr(process.readFd) << " to _tmpCgiFds" << std::endl;
            _tmpCgiFds.insert(process.readFd);
            log.debug() << "Removing remote client " << repr(clientSocket) << " from persistent connections" << std::endl;
            persistConns.erase(clientSocket);
            // closeAndRemoveMultPlexFd(_monitorFds, clientSocket);
            // _pendingWrites.erase(clientSocket);
        } else {
            log.debug() << "CGI process " << repr(process) << " is not timed out" << std::endl;
            if (process.dead) {
                if (process.noRecentReadEvent) {
                    log.debug() << "Process is dead and doesn't have read events: " << repr(process) << std::endl;
                    if (process.response.empty() && !process.headersSent) {
                        log.trace() << "Process died, response is empty and headers have not yet been sent, sending 502 Bad Gateway"
                                    << std::endl;
                        sendError(clientSocket, 502, process.location);
                    } else if (process.response.empty() && process.headersSent) {
                        log.trace()
                            << "Process died, response is empty, BUT headers have been sent/enqueued, not sending anything extra/any errors"
                            << std::endl;
                    } else if (!process.response.empty()) { // not perfect, we should check if it's a valid response, i.e. headers present,
                                                            // \r\n\r\n, etc., but it'll do
                        log.trace()
                            << "Process died but there's still response data to enqueue, sending that data raw (no header validation)"
                            << std::endl;
                        queueWrite(clientSocket, _httpVersionString + " 200 " + statusTextFromCode(200) + "\r\n" + process.response);
                        log.trace() << "Marking process as done: " << repr(process) << std::endl;
                        process.done = true;
                    }
                    log.debug() << "Removing remote client " << repr(clientSocket) << " from persistent connections" << std::endl;
                    persistConns.erase(clientSocket);
                    closeAndRemoveMultPlexFd(_monitorFds, process.readFd);
                    log.debug() << "Removing its read FD " << repr(process.readFd) << " (stdout) from cgiToClient map" << std::endl;
                    deleteFromCgiToClient.push_back(process.readFd);
                    deleteFromClientToCgi.push_back(clientSocket);
                    // _pendingWrites.erase(clientSocket);
                } else {
                    log.debug() << "Process is dead, but still had a recent read event" << std::endl;
                }
            } else {
                log.debug() << "Proces is not dead" << std::endl;
            }
        }

        if (::waitpid(process.pid, NULL, WNOHANG) > 0) {
            log.debug() << "Reaped CGI process " << repr(process) << std::endl;
            log.debug() << "Updating its dead state to " << repr(true) << std::endl;
            process.dead = true;
        }
        process.noRecentReadEvent = true; // should be reset to false if there was a read event
    }
    for (size_t i = 0; i < deleteFromClientToCgi.size(); ++i) {
        _clientToCgi.erase(deleteFromClientToCgi[i]);
        _cgiToClient.erase(deleteFromCgiToClient[i]);
    }
}
