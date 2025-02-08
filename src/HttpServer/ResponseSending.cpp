#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <ostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Config.hpp"
#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

using Utils::STR;

void HttpServer::queueWrite(int clientSocket, const string &data) {
    log.debug() << "Queueing a write: " << repr(data) << std::endl;
    if (_pendingWrites.find(clientSocket) == _pendingWrites.end())
        _pendingWrites[clientSocket] = string(data);
    else
        _pendingWrites[clientSocket] += data;
    startMonitoringForWriteEvents(_monitorFds, clientSocket);
    log.debug() << "Pending writes is now: " << repr(_pendingWrites) << std::endl;
}

void HttpServer::terminatePendingCloses(int clientSocket) {
    if (_pendingCloses.find(clientSocket) != _pendingCloses.end()) {
        log.debug() << "Removing client " << repr(clientSocket) << " from pendingCloses" << std::endl;
        _pendingCloses.erase(clientSocket);
        removeClient(clientSocket);
    } else {
        log.debug() << "Client " << repr(clientSocket) << " is not in pendingCloses, not doing anything with it" << std::endl;
    }
}

bool HttpServer::maybeTerminateConnection(PendingWriteMap::iterator it, int clientSocket) {
    if (it == _pendingWrites.end()) {                                // no pending writes anymore, maybe close
        if (_clientToCgi.find(clientSocket) == _clientToCgi.end()) { // STOP, make sure CGI doesn't want to write data to this client
            if (_cgiToClient.count(clientSocket) == 0) {             // STOP, make sure clientSocket is not a writeFd to the CGI process
                log.debug() << "Client " << repr(clientSocket)
                            << " is a proper remote client, there are no pending writes "
                               "for this client and no CGI process wants to write data "
                               "to it. Closing connection to it."
                            << std::endl;
                // No pending writes, for POLL: remove POLLOUT from events
                // TODO: @timo: is this really needed? aren't we removing the client
                // anyways?
                stopMonitoringForWriteEvents(_monitorFds, clientSocket);
                // Also if this client's connection is pending to be closed, close it
                terminatePendingCloses(clientSocket);
            } else {
                log.debug() << "There are no pending writes to FD " << repr(clientSocket)
                            << " anymore, but don't terminate"
                               " connection, since FD refers to the stdin of a CGI "
                               "process and will be cleaned up elsewhere"
                            << std::endl;
            }
        } else
            log.debug() << "There are no pending writes to FD " << repr(clientSocket)
                        << " anymore, but don't terminate "
                           "connection yet, since the following CGI process still wants to write "
                           "data to the client: "
                        << repr(_clientToCgi.at(clientSocket)) << std::endl;
        return true;
    }
    log.debug() << "There are still pending writes, not terminating connection to client " << repr(clientSocket) << std::endl;
    return false;
}

void HttpServer::terminateIfNoPendingDataAndNoCgi(PendingWriteMap::iterator &it, int clientSocket, ssize_t bytesSent) {
    PendingWrite &pw = it->second;

    // Check whether we've sent everything and that there are no CGI processes
    if (_clientToCgi.find(clientSocket) == _clientToCgi.end() && (pw.length() == 0 || bytesSent == 0)) {
        log.debug() << "Removing client " << repr(clientSocket) << " from pendingWrites" << std::endl;
        _pendingWrites.erase(it);
        // TODO: @all: what about removing pendingCloses?
        // for POLL: remove POLLOUT from events since we're done writing
        stopMonitoringForWriteEvents(_monitorFds, clientSocket);
        // TODO: @all: yeah actually we really have to close the connection here somehow,
        // removeCLient or smth
        removeClient(clientSocket); // REALLY NOT SURE ABOUT THIS ONE
        log.debug() << "Removing client " << repr(clientSocket) << " from pendingCloses" << std::endl;
        _pendingCloses.erase(clientSocket); // also not 100% sure about this one
        if (_cgiToClient.count(clientSocket) != 0) {
            log.debug() << "Removing CGI pipe FD " << repr(clientSocket) << " (write end, stdin of CGI) from cgiToClient map" << std::endl;
            _cgiToClient.erase(clientSocket);
        }
    }
}

void HttpServer::writeToClient(int clientSocket) {
    PendingWriteMap::iterator it = _pendingWrites.find(clientSocket);
    if (maybeTerminateConnection(it, clientSocket))
        return;

    PendingWrite &pw = it->second;
    size_t dataSize = std::min(Constants::chunkSize, pw.length());
    string dataChunk = pw.substr(0, dataSize);
    pw = pw.substr(dataSize); // remove chunk (that will be send) from the beginning
    const char *data = dataChunk.c_str();

    ssize_t bytesSent;
    if (_cgiToClient.count(clientSocket)) { // it's a writeFd for the CGI, can't use send
        log.debug() << "Writing this data of " << repr(dataSize) << " bytes to socket " << repr(clientSocket) << ": " << repr((char *)data) << std::endl;
        bytesSent = ::write(clientSocket, data, dataSize);
        log.debug() << "Written " << repr(bytesSent) << " bytes to client " << repr(clientSocket) << std::endl;
    } else {
        log.debug() << "Sending this data of " << repr(dataSize) << " bytes to socket " << repr(clientSocket) << ": " << repr((char *)data) << std::endl;
        bytesSent = ::send(clientSocket, data, dataSize, 0);
        log.debug() << "Sent " << repr(bytesSent) << " bytes to client " << repr(clientSocket) << std::endl;
    }

    if (bytesSent < 0) {
        // TODO: @all: remove pending closes? clear pending writes?
        removeClient(clientSocket);
        log.debug() << "Removing client " << repr(clientSocket) << " from pendingWrites" << std::endl;
        _pendingWrites.erase(clientSocket);
        log.debug() << "Removing client " << repr(clientSocket) << " from pendingCloses" << std::endl;
        _pendingCloses.erase(clientSocket);
        if (_cgiToClient.count(clientSocket) > 0) {
            int client = _cgiToClient[clientSocket];
            log.debug() << "Client " << repr(client) << " had data to send but pipe broke" << std::endl;
            log.debug() << "Notifying remote client " << repr(client) << " with a " << repr(500) << ", since CGI process died/closed stdin" << std::endl;
            sendError(client, 502, NULL);
        }
        return;
    }

    log.debug() << "Successfully sent data to " << repr(clientSocket) << ", now checking if we need to reset the connection" << std::endl;
    terminateIfNoPendingDataAndNoCgi(it, clientSocket, bytesSent);
}

// note, be careful sending errors from this function, as it could lead to infinite
// recursion! (the error sending function uses this function)
bool HttpServer::sendFileContent(int clientSocket, const string &filePath, const LocationCtx &location, int statusCode, const string &contentType, bool onlyHeaders) {
    log.debug() << "Trying to send file content of file " << repr(filePath) << std::endl;
    std::ifstream file(filePath.c_str(), std::ifstream::binary);
    if (!file) {
        log.debug() << "Could not open file " << repr(filePath) << std::endl;
        sendError(clientSocket, 403, &location);
        return false;
    }

    log.debug() << "Could open file " << repr(filePath) << ", determining file size via istream::seekg and istream::tellg" << std::endl;
    file.seekg(0, std::ios::end);
    long fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    log.debug() << "File size is " << repr(fileSize) << std::endl;

    std::ostringstream headers;
    log.debug() << "Building headers for error response" << std::endl;
    headers << _httpVersionString << " " << statusCode << " " << statusTextFromCode(statusCode) << "\r\n"
            << "Content-Length: " << fileSize << "\r\n"
            << "Content-Type: " << (contentType.empty() ? getMimeType(filePath) : contentType) << "\r\n"
            << "Connection: close\r\n\r\n";

    queueWrite(clientSocket, headers.str());

    if (!onlyHeaders) {
        log.debug() << "Queueing actual file contents, since it's not a HEAD request" << std::endl;
        char buffer[CONSTANTS_CHUNK_SIZE];
        while (file.read(buffer, sizeof(buffer)))
            queueWrite(clientSocket, string(buffer, static_cast<size_t>(file.gcount())));
        if (file.gcount() > 0)
            queueWrite(clientSocket, string(buffer, static_cast<size_t>(file.gcount())));
    } else {
        log.debug() << "It was a HEAD request, not queuing any file contents" << std::endl;
    }
    log.debug() << "Adding socket " << repr(clientSocket) << " to pendingCloses" << std::endl;
    _pendingCloses.insert(clientSocket);
    return true;
}

string HttpServer::wrapInHtmlBody(const string &text) {
    log.trace() << "Wrapping text in some html and body tags: " << repr(text) << std::endl;
    return "<html>\r\n\t<body>\r\n\t\t" + text + "\r\n\t</body>\r\n</html>\r\n";
}

static string getErrorPagePath(int statusCode, const LocationCtx &location) {
    Logger::lastInstance().debug() << "Getting error page path for status code " << repr(statusCode) << " and location " << repr(location) << std::endl;
    ArgResults allErrPageDirectives = getAllDirectives(location.second, "error_page");
    for (ArgResults::const_iterator errPages = allErrPageDirectives.begin(); errPages != allErrPageDirectives.end(); ++errPages) {
        Arguments::const_iterator code = errPages->begin(), before = --errPages->end();
        for (; code != before; ++code) {
            if (std::atoi(code->c_str()) == statusCode) {
                Logger::lastInstance().debug() << "Returning error page path for status code " << repr(statusCode) << ": " << repr(errPages->back()) << std::endl;
                return errPages->back();
            }
        }
    }
    Logger::lastInstance().debug() << "Status code " << repr(statusCode) << " has no error page, returning empty string" << std::endl;
    return "";
}

bool HttpServer::sendErrorPage(int clientSocket, int statusCode, const LocationCtx &location) {
    log.debug() << "Trying to send error page to client " << repr(clientSocket) << " with status code " << repr(statusCode) << " and location block " << repr(location) << std::endl;
    string errorPageLocation = getErrorPagePath(statusCode, location);
    if (errorPageLocation.empty())
        return false;
    string errorPagePath = getFirstDirective(location.second, "root")[0] + errorPageLocation;
    log.debug() << "Full file system error page path is " << repr(errorPagePath) << ", trying to open that file" << std::endl;
    std::ifstream file(errorPagePath.c_str());
    if (!file) {
        log.debug() << "Failed to open " << repr(errorPagePath) << std::endl;
        return false;
    }
    log.debug() << "Can open " << repr(errorPagePath) << ", so all good" << std::endl;
    return sendFileContent(clientSocket, errorPagePath, location, statusCode);
}

// pass NULL as location if errorPages should not be served
void HttpServer::sendError(int clientSocket, int statusCode, const LocationCtx *const location) {
    if (location == NULL || !sendErrorPage(clientSocket, statusCode, *location)) {
        if (location == NULL)
            log.debug() << "Location was " << num("NULL") << std::endl;
        else
            log.debug() << "Couldn't send error page" << std::endl;
        log.debug() << "Sending hardcoded error with status code: " << repr(statusCode) << std::endl;
        string errorContent = wrapInHtmlBody("<h1>\r\n\t\t\t" + STR(statusCode) + " " + statusTextFromCode(statusCode) + "\r\n\t\t</h1>");
        sendString(clientSocket, errorContent, statusCode);
    }
}

void HttpServer::sendString(int clientSocket, const string &payload, int statusCode, const string &contentType, bool onlyHeaders) {
    log.debug() << "Preparing to send string response with status code: " << repr(statusCode) << std::endl;
    log.trace() << "The payload is " << repr(payload) << std::endl;
    std::ostringstream response;

    response << _httpVersionString << " " << statusCode << " " << statusTextFromCode(statusCode) << "\r\n"
             << "Content-Type: " << contentType << "\r\n"
             << "Content-Length: " << payload.length() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n";
    if (!onlyHeaders) {
        log.debug() << "Queueing actual string payload, since it's not a HEAD request" << std::endl;
        response << payload;
    } else {
        log.debug() << "It was a HEAD request, not queuing any payload" << std::endl;
    }

    queueWrite(clientSocket, response.str());
    log.debug() << "Adding socket " << repr(clientSocket) << " to pendingCloses" << std::endl;
    _pendingCloses.insert(clientSocket);
}
