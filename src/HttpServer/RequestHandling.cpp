#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <ostream>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "CgiHandler.hpp"
#include "Config.hpp"
#include "Constants.hpp"
#include "DirectiveValidation.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

using std::runtime_error;

bool HttpServer::requestIsForCgi(const HttpRequest &request, const LocationCtx &location) {
    log.debug() << "Checking if request is for CGI" << std::endl;
    log.trace() << "Request is " << repr(request) << std::endl;
    if (!directiveExists(location.second, "cgi_dir")) {
        log.debug() << "Request is not for CGI, because cgi_dir directive is missing from the location: "
                    << repr(location) << std::endl;
        return false;
    }
    string uri = request.path;
    string cgi_dir = getFirstDirective(location.second, "cgi_dir")[0];
    if (!Utils::isPrefix(cgi_dir, uri)) {
        log.debug() << "Request is not for CGI, because cgi_dir directive " << cgi_dir
                    << " is not a prefix of the request path: " << uri << std::endl;
        return false;
    }
    log.debug() << "Request IS for CGI, because cgi_dir directive " << cgi_dir
                << " is a prefix of the request path: " << uri << std::endl;
    return true;
}

void HttpServer::handleCgiRead(int cgiFd) {

    int clientSocket = _cgiToClient[cgiFd]; // guaranteed to work because of the checks before this function is called
    ClientFdToCgiMap::iterator it = _clientToCgi.find(clientSocket);
    if (it == _clientToCgi.end()) {
        // CGI process not found
        sendError(cgiFd, 502, NULL);
        return;
    }
    CgiProcess &process = it->second;

    if (_cgiToClient.count(process.writeFd) > 0 && _cgiToClient[process.writeFd] == clientSocket)
        return; // there's still pending writes (from client POST) to this CGI process

    char buffer[CONSTANTS_CHUNK_SIZE];
    ssize_t bytesRead = ::read(cgiFd, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) {
        ::kill(process.pid, SIGKILL);
        sendError(process.clientSocket, 502, process.location);
        // TODO: @all: not sure if the following line is correct
        closeAndRemoveMultPlexFd(_monitorFds, cgiFd);
        _clientToCgi.erase(clientSocket); // TODO: @all couple _cgiToClient and _clientToCgi as as to never forget to
                                          // remove one of them if the other is removed
        _cgiToClient.erase(cgiFd);
        return;
    }

    if (bytesRead == 0) { // EOF - CGI process finished writing
        int status;
        // TODO: @all: we should install a signal handler that will reap the CGI process
        ::waitpid(process.pid, &status, WNOHANG);

        if (!process.headersSent)
            sendError(process.clientSocket, 502, process.location);
        else if (!process.response.empty())
            queueWrite(process.clientSocket, process.response);

        closeAndRemoveMultPlexFd(_monitorFds, cgiFd);
        _clientToCgi.erase(clientSocket);
        _cgiToClient.erase(cgiFd);
        return;
    }

    process.lastActive = std::time(NULL);
    buffer[bytesRead] = '\0';
    process.totalSize += static_cast<unsigned long>(bytesRead);

    if (process.totalSize > 10 * 1024 * 1024) { // 10MB limit
        ::kill(process.pid, SIGKILL);
        sendError(process.clientSocket, 413, process.location);
        // TODO: @all: not sure if the following line is correct
        closeAndRemoveMultPlexFd(_monitorFds, cgiFd);
        _clientToCgi.erase(clientSocket);
        _cgiToClient.erase(cgiFd);
        return;
    }

    if (!process.headersSent) {
        process.response += string(buffer, static_cast<size_t>(bytesRead));
        size_t headerEnd = process.response.find("\r\n\r\n");
        if (headerEnd != string::npos) {
            // Found headers, prepare full response with HTTP/1.1
            string headers = process.response.substr(0, headerEnd);
            string body = process.response.substr(headerEnd + 4);

            std::ostringstream fullResponse;
            fullResponse << "HTTP/1.1 200 OK\r\n";

            // Add Content-Type if not present
            if (headers.find("Content-Type:") == string::npos) {
                fullResponse << "Content-Type: text/html\r\n";
            }

            fullResponse << headers << "\r\n\r\n" << body;

            process.headersSent = true;
            queueWrite(process.clientSocket, fullResponse.str());
            process.response.clear();

        } else if (process.response.length() > 8192) { // Headers too long
            ::kill(process.pid, SIGKILL);
            sendError(process.clientSocket, 502, process.location);
            // TODO: @all: not sure if the following line is correct
            closeAndRemoveMultPlexFd(_monitorFds, cgiFd);
            _clientToCgi.erase(
                clientSocket);         // TODO: @timo: rename _clientToCgi to something like _clientFdToCgiProcess
            _cgiToClient.erase(cgiFd); // TODO: @timo: rename _cgiToClient to something like _cgiFdToClientFd
            return;
        }
    } else {
        queueWrite(process.clientSocket, string(buffer, static_cast<size_t>(bytesRead)));
    }
}

bool HttpServer::parseRequestLine(const string &line, HttpRequest &request) {
    std::istringstream iss(line);
    if (!(iss >> request.method >> request.path >> request.httpVersion)) {
        return false;
    }

    request.path = canonicalizePath(request.path);
    request.pathParsed = true;
    return true;
}

bool HttpServer::parseHeader(const string &line, HttpRequest &request) {
    size_t colonPos = line.find(':');
    if (colonPos == string::npos) {
        return false;
    }

    string key = line.substr(0, colonPos);
    string value = line.substr(colonPos + 1);

    // Trim leading/trailing whitespace
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t\r\n") + 1);

    request.headers[key] = value;
    return true;
}

void HttpServer::parseHeaders(std::istringstream &requestStream, string &line, HttpRequest &request) {
    while (std::getline(requestStream, line) &&
           line != "\r") { // TODO: @all: headers might be very hard to parse
                           // (multiline, etc) // RFC should really be the reference
        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            string key = line.substr(0, colonPos);
            string value = line.substr(colonPos + 1);
            value.erase(0, value.find_first_not_of(" "));
            value.erase(value.find_last_not_of("\r") + 1);
            request.headers[key] = value;
        }
    }
}

HttpServer::HttpRequest HttpServer::parseHttpRequest(const char *buffer) {
    HttpRequest request;
    std::istringstream requestStream(buffer);
    string line;

    std::getline(requestStream, line);

    std::istringstream requestLine(line);
    requestLine >> request.method >> request.path >> request.httpVersion;
    request.path = canonicalizePath(request.path);

    parseHeaders(requestStream, line, request);
    return request;
}

void HttpServer::handleDelete(int clientSocket, const HttpRequest &request, const LocationCtx &location) {
    if (!directiveExists(location.second, "upload_dir"))
        sendError(clientSocket, 405, NULL);
    string diskPath = determineDiskPath(request, location);

    struct stat fileStat;
    int fileExists = stat(diskPath.c_str(), &fileStat) == 0;

    if (!fileExists) {
        sendError(clientSocket, 404, &location);
    } else if (S_ISDIR(fileStat.st_mode)) {
        sendString(clientSocket, "Directory deletion is turned off", 403);
    } else if (S_ISREG(fileStat.st_mode)) {
        if (::remove(diskPath.c_str()) < 0)
            sendString(clientSocket, "Failed to delete resource " + request.path, 500);
        else
            sendString(clientSocket, "Successfully deleted " + request.path + "\n");
    } else {
        sendString(clientSocket, "You may only delete regular files", 403);
    }
}

static string getFileName(const string &path) {
    size_t pos;
    if ((pos = path.rfind('/')) == path.npos)
        return path;
    return path.substr(pos);
}

void HttpServer::handleUpload(int clientSocket, const HttpRequest &request, const LocationCtx &location,
                              bool overwrite) {
    if (!directiveExists(location.second, "upload_dir"))
        sendError(clientSocket, 405, NULL);
    string fileName = getFileName(request.path);
    string diskPath =
        getFirstDirective(location.second, "root")[0] + getFirstDirective(location.second, "upload_dir")[0] + fileName;

    struct stat fileStat;
    int fileExists = stat(diskPath.c_str(), &fileStat) == 0;

    if (fileExists && !overwrite)
        sendError(clientSocket, 409, &location); // == conflict

    std::ofstream of(diskPath.c_str());
    if (!of) {
        sendError(clientSocket, 500, &location);
        return;
    }
    of.write(request.body.c_str(), static_cast<long>(request.body.length()));
    of.flush();
    of.close();
    sendString(clientSocket, "Successfully uploaded " + request.path + "\n", 201);
}

void HttpServer::handleRequestInternally(int clientSocket, const HttpRequest &request, const LocationCtx &location) {
    if (request.method == "GET" || request.method == "HEAD")
        serveStaticContent(clientSocket, request, location);
    else if (request.method == "POST")
        handleUpload(clientSocket, request, location, false);
    else if (request.method == "PUT")
        handleUpload(clientSocket, request, location, true);
    else if (request.method == "DELETE")
        handleDelete(clientSocket, request, location);
    else if (request.method == "FTFT")
        sendString(clientSocket, repr(*this) + '\n');
    else {
        sendError(clientSocket, 405, &location);
    }
}

bool HttpServer::methodAllowed(const HttpRequest &request, const LocationCtx &location) {
    if (!Utils::allUppercase(request.method)) {
        log.debug() << "Method not allowed, not all uppercase" << std::endl;
        return false;
    } else if (!directiveExists(location.second, "limit_except")) {
        log.debug() << "Method maybe allowed, limit_except directive doesn't exist and request is all uppercase"
                    << std::endl;
        return true;
    }
    string limit_except = "limit_except";
    const Arguments &allowedMethods = getFirstDirective(location.second, limit_except);
    log.debug() << "Checking if method is one of " << repr(allowedMethods) << std::endl;
    for (Arguments::const_iterator method = allowedMethods.begin(); method != allowedMethods.end(); ++method) {
        if (*method == request.method) {
            log.debug() << "Method allowed, since " << *method << " == " << request.method << std::endl;
            return true;
        }
    }
    log.debug() << "Method not allowed, since it was not one of the allowed directives" << std::endl;
    return false;
}

void HttpServer::rewriteRequest(int clientSocket, int statusCode, const string &urlOrText,
                                const LocationCtx &location) {
    if (DirectiveValidation::isDoubleQuoted(urlOrText))
        sendString(clientSocket, DirectiveValidation::decodeDoubleQuotedString(urlOrText), statusCode);
    else if (DirectiveValidation::isHttpUri(urlOrText))
        redirectClient(clientSocket, urlOrText, statusCode);
    else
        sendError(clientSocket, 500, &location); // proper parsing should catch this case!
}

// MAYBE: refactor
void HttpServer::handleRequest(int clientSocket, const HttpRequest &request, const LocationCtx &location) {
    log.info() << "Handling request: " << repr(request) << " to location " << repr(location) << std::endl;
    if (!methodAllowed(request, location))
        sendError(clientSocket, 405, &location);
    else if (directiveExists(location.second, "return"))
        rewriteRequest(clientSocket, std::atoi(getFirstDirective(location.second, "return")[0].c_str()),
                       getFirstDirective(location.second, "return")[1], location);
    else if (!requestIsForCgi(request, location))
        handleRequestInternally(clientSocket, request, location);
    else
        try {
            ArgResults cgiExts = getAllDirectives(location.second, "cgi_ext");
            for (ArgResults::const_iterator ext = cgiExts.begin(); ext != cgiExts.end(); ++ext) {
                string extension = (*ext)[0];
                string program = ext->size() > 1 ? (*ext)[1] : "";

                CgiHandler handler(*this, extension, program, log);
                if (handler.canHandle(request.path)) {
                    handler.execute(clientSocket, request, location);
                    return;
                }
            }

            // No matching CGI handler found
            sendError(clientSocket, 404, &location);
        } catch (const std::exception &e) {
            log.error() << string("CGI execution failed: ") << e.what() << std::endl;
            sendError(clientSocket, 500, &location);
        }
}

ssize_t HttpServer::recvToBuffer(int clientSocket, char *buffer, size_t bufSiz) {
    ssize_t bytesRead = recv(clientSocket, buffer, bufSiz, 0);

    if (bytesRead <= 0) {
        if (bytesRead == 0)
            removeClient(clientSocket);
        else
            log.error() << string("recv failed: ") << strerror(errno) << std::endl;
        return 0;
    }

    buffer[bytesRead] = '\0';
    return bytesRead;
}

bool HttpServer::isHeaderComplete(const HttpRequest &request) const {
    return !request.method.empty() && !request.path.empty() && !request.httpVersion.empty();
}

bool HttpServer::needsMoreData(const HttpRequest &request) const {
    if (request.state == READING_HEADERS)
        return true;
    if (request.state == READING_BODY) {
        if (request.chunkedTransfer)
            return true; // TODO: @sonia Will be handled in follow-up PR
        return request.bytesRead < request.contentLength;
    }
    return false;
}

void HttpServer::processContentLength(HttpRequest &request) {
    if (request.headers.find("Content-Length") != request.headers.end())
        request.contentLength = static_cast<size_t>(std::atoi(request.headers["Content-Length"].c_str()));

    if (request.headers.find("Transfer-Encoding") != request.headers.end() &&
        request.headers["Transfer-Encoding"] == "chunked")
        request.chunkedTransfer = true;
}

bool HttpServer::validateRequest(const HttpRequest &request, int clientSocket) {
    // First check headers are complete
    if (!isHeaderComplete(request)) {
        log.debug() << "Invalid request: incomplete headers" << std::endl;
        sendError(clientSocket, 400, NULL);
        return false;
    }

    // Check HTTP method
    if (request.method != "GET" && request.method != "HEAD" && request.method != "POST" && request.method != "DELETE" &&
        request.method != "PUT" && request.method != "FTFT") {
        log.debug() << "Invalid request: unsupported method: " << request.method << std::endl;
        sendError(clientSocket, 405, NULL);
        return false;
    }

    // Check HTTP version
    if (request.httpVersion != "HTTP/1.1") {
        log.debug() << "Invalid request: unsupported HTTP version: " << request.httpVersion << std::endl;
        sendError(clientSocket, 505, NULL);
        return false;
    }

    // For POST requests, verify content info
    if (request.method == "POST" && !request.chunkedTransfer && request.contentLength == 0) {
        log.debug() << "Invalid POST request: no content length or chunked transfer" << std::endl;
        sendError(clientSocket, 400, NULL);
        return false;
    }

    return true;
}

size_t HttpServer::getRequestSizeLimit(int clientSocket, const HttpRequest &request) {
    try {
        const LocationCtx &loc = requestToLocation(clientSocket, request);
        return Utils::convertSizeToBytes(getFirstDirective(loc.second, "client_max_body_size")[0]);
    } catch (const runtime_error &error) {
        log.error() << error.what() << std::endl;
        return Utils::convertSizeToBytes("1m");
    }
}

void HttpServer::removeClientAndRequest(int clientSocket) {
    removeClient(clientSocket);
    _pendingRequests.erase(clientSocket);
}

bool HttpServer::checkRequestSize(int clientSocket, const HttpRequest &request, size_t currentSize) {
    size_t sizeLimit = getRequestSizeLimit(clientSocket, request);
    if (currentSize > sizeLimit) {
        sendError(clientSocket, 413, NULL); // Payload Too Large
        // TODO: @discuss: what about removing clientSocket from _pendingRequests
        // removeClientAndRequest(clientSocket);
        _pendingRequests.erase(clientSocket);
        return false;
    }
    return true;
}

bool HttpServer::processRequestHeaders(int clientSocket, HttpRequest &request, const string &rawData) {
    size_t headerEnd = rawData.find("\r\n\r\n");
    log.debug() << "Looking for headers end marker. Found: " << (headerEnd != string::npos) << std::endl;
    if (headerEnd == string::npos) {
        return false; // Need more data
    }

    std::istringstream stream(rawData);
    string line;

    // Parse request line (GET /path HTTP/1.1)
    if (!std::getline(stream, line) || !parseRequestLine(line, request)) {
        log.debug() << "Failed to parse request line: [" << Utils::escape(line) << "]" << std::endl;
        sendError(clientSocket, 400, NULL);
        _pendingRequests.erase(clientSocket);
        return false;
    }
    log.debug() << "Parsed request line: " << request.method << " " + request.path << std::endl;
    // Parse headers (Key: Value)
    while (std::getline(stream, line) && line != "\r") {
        if (!parseHeader(line, request)) {
            log.debug() << "Failed to parse header line: [" << Utils::escape(line) << "]" << std::endl;
            sendError(clientSocket, 400, NULL);
            _pendingRequests.erase(clientSocket);
            return false;
        }
        log.debug() << "Parsed header line: [" << Utils::escape(line) << "]" << std::endl;
    }

    // Process Content-Length and chunked transfer headers
    processContentLength(request);
    log.debug() << "Content length detected: " << request.contentLength << std::endl;
    log.debug() << "Chunked transfer: " << request.chunkedTransfer << std::endl;

    // Validate the request
    if (!validateRequest(request, clientSocket)) {
        log.debug() << "Request validation failed" << std::endl;
        // TODO: @discuss: what about removing clientSocket from _pendingRequests
        // removeClientAndRequest(clientSocket);
        _pendingRequests.erase(clientSocket);
        return false;
    }

    // Move any remaining data to body
    if (headerEnd + 4 < rawData.size()) {
        request.body = rawData.substr(headerEnd + 4);
        request.bytesRead = request.body.size();
        log.debug() << "Moved " << request.bytesRead << " bytes to body" << std::endl;
    } else {
        request.body.clear();
        request.bytesRead = 0;
    }

    // Update state
    request.state = READING_BODY;

    // If no body expected, mark as complete
    if (!request.chunkedTransfer && (request.contentLength == 0 || request.bytesRead >= request.contentLength)) {
        log.debug() << "No body expected, marking as complete" << std::endl;
        request.state = REQUEST_COMPLETE;
    }

    return true;
}

bool HttpServer::processRequestBody(int clientSocket, HttpRequest &request, const char *buffer, size_t bytesRead) {
    // Append new data to body
    request.body.append(buffer, bytesRead);
    request.bytesRead += bytesRead;

    // Check size limit
    if (!checkRequestSize(clientSocket, request, request.body.size()))
        return false;

    // Check if we have all the data
    if (!request.chunkedTransfer && request.bytesRead >= request.contentLength)
        request.state = REQUEST_COMPLETE;

    return true;
}

void HttpServer::finalizeRequest(int clientSocket, HttpRequest &request) {
    bool bound = false;
    try {
        const LocationCtx &location = requestToLocation(clientSocket, request);
        bound = true;
        log.debug() << "Found location " << repr(location) << " for request " << repr(request) << std::endl;
        handleRequest(clientSocket, request, location);
    } catch (const runtime_error &err) {
        if (!bound)
            throw; // requestToLocation may throw, but if handleRequest throws, we got bigger problems, thus, rethrowing
        log.error() << err.what() << std::endl;
    }
    _pendingRequests.erase(clientSocket);
}

void HttpServer::handleIncomingData(int clientSocket, const char *buffer, ssize_t bytesRead) {

    log.debug() << "Received data length: " << bytesRead << std::endl;
    log.debug() << "Raw data: [" << Utils::escape(string(buffer, static_cast<size_t>(bytesRead))) << "]" << std::endl;
    // Get or create request state
    HttpRequest &request = _pendingRequests[clientSocket];
    log.debug() << "Current request state: " << static_cast<int>(request.state) << std::endl;

    // Process based on current state
    if (request.state == READING_HEADERS) {
        // During header reading, we temporarily accumulate data
        string rawData = request.temporaryBuffer + string(buffer, static_cast<size_t>(bytesRead));
        log.debug() << "Accumulated data length: " << rawData.size() << std::endl;

        // Check accumulated size
        // DONT check in headers, must only check body size
        // if (request.pathParsed && !checkRequestSize(clientSocket, request, rawData.size()))
        // 	return;

        // Store for next iteration if headers aren't complete
        request.temporaryBuffer = rawData;

        // Try to process headers
        if (processRequestHeaders(clientSocket, request, rawData)) {
            log.debug() << "Headers processed. Body size so far: " << request.body.size() << std::endl;
            log.debug() << "Expected content length: " << request.contentLength << std::endl;
            request.temporaryBuffer.clear();
        }
    } else if (request.state == READING_BODY) {
        log.debug() << "Reading body. Current size: " << request.bytesRead << " Expected: " << request.contentLength
                    << std::endl;
        if (!processRequestBody(clientSocket, request, buffer, static_cast<size_t>(bytesRead))) {
            return;
        }
    }
    // no else if. if state was set to complete just now then might as well already handle it in the same pass
    if (request.state == REQUEST_COMPLETE) { // If request is complete, handle it
        if (!checkRequestSize(clientSocket, request, request.body.length()))
            return;

        log.debug() << "Request complete. Processing..." << std::endl;
        finalizeRequest(clientSocket, request);
    }
}

void HttpServer::readFromClient(int clientSocket) {
    if (_cgiToClient.find(clientSocket) != _cgiToClient.end()) {
        handleCgiRead(clientSocket);
        return;
    }

    char buffer[CONSTANTS_CHUNK_SIZE];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        removeClientAndRequest(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0';
    handleIncomingData(clientSocket, buffer, bytesRead);
}
