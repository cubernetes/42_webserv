#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <ios>
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

#include "Ansi.hpp"
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
        log.debug() << "Request is not for CGI, because cgi_dir directive is missing "
                       "from the location: "
                    << repr(location) << std::endl;
        return false;
    }
    string path = request.path;
    string cgi_dir = getFirstDirective(location.second, "cgi_dir")[0];
    if (!Utils::isPrefix(cgi_dir, path)) {
        log.debug() << "Request is not for CGI, because cgi_dir directive " << repr(cgi_dir)
                    << " is not a prefix of the request path: " << repr(path) << std::endl;
        return false;
    }
    log.debug() << "Request IS for CGI, because cgi_dir directive " << repr(cgi_dir) << " is a prefix of the request path: " << repr(path)
                << std::endl;
    return true;
}

void HttpServer::handleCgiRead(int cgiFd) {
    int clientSocket = _cgiToClient[cgiFd]; // guaranteed to work because of the checks
                                            // before this function is called
    log.debug() << "Pipe FD for the stdout of the CGI process: " << repr(cgiFd) << std::endl;
    log.debug() << "Remote client socket FD for the CGI process: " << repr(clientSocket) << std::endl;
    ClientFdToCgiMap::iterator it = _clientToCgi.find(clientSocket);
    if (it == _clientToCgi.end()) {
        // CGI process not found, should never happen
        log.debug() << "During invalidation of cgiToClient map, CGI process "
                       "doesn't exist anymore, cleaning up"
                    << std::endl;
        _cgiToClient.erase(cgiFd);
        // sendError(cgiFd /* cgiFd??? doesn't make sense */, 502, NULL);
        return;
    }
    CgiProcess &process = it->second;
    process.noRecentReadEvent = false;
    log.debug() << "CGI process PID: " << repr(process.pid) << std::endl;
    log.debug() << "Pipe FD for the stdin of the CGI process: " << repr(process.writeFd) << std::endl;

    if (_cgiToClient.count(process.writeFd) > 0 && _cgiToClient[process.writeFd] == clientSocket) {
        log.debug() << "Client still has data to send, let's handle the read only when "
                       "all data has been sent (half-duplex)"
                    << std::endl;
        return; // there's still pending writes (from client POST) to this CGI process
    }

    char buffer[CONSTANTS_CHUNK_SIZE + 1];
    log.debug() << "Calling " << func("read") << punct("()") << " on the CGI pipe FD " << repr(cgiFd) << " for " << repr(sizeof(buffer))
                << " bytes" << std::endl;
    ssize_t bytesRead = ::read(cgiFd, buffer, sizeof(buffer) - 1);
    log.debug() << "Actually read " << repr(bytesRead) << " bytes" << std::endl;
    if (bytesRead < 0) {
        log.debug() << "Error while reading from the CGI pipe FD " << repr(cgiFd) << ": " << ::strerror(errno) << std::endl;
        log.debug() << "Sending " << num("SIGKILL") << " using " << func("kill") << punct("()") << " to the CGI process with PID "
                    << repr(process.pid) << std::endl;
        ::kill(process.pid, SIGKILL);
        sendError(clientSocket, 502, process.location);
        log.debug() << "Removing remote client " << repr(clientSocket) << " from persistent connections" << std::endl;
        persistConns.erase(clientSocket);
        closeAndRemoveMultPlexFd(_monitorFds, cgiFd);
        log.debug() << "Removing remote client " << repr(clientSocket) << " from clientToCgi map" << std::endl;
        _clientToCgi.erase(clientSocket); // NOTODO: @all couple _cgiToClient and _clientToCgi as as to
        log.debug() << "Removing CGI pipe FD " << repr(cgiFd) << " from cgiToClient map" << std::endl;
        _cgiToClient.erase(cgiFd);
        return;
    }

    if (bytesRead == 0) { // EOF - CGI process finished writing
        log.debug() << "Read 0 bytes from the CGI pipe FD, which is odd since I/O "
                       "multiplexing said there's a read event, but whatever"
                    << std::endl;

        // not sure, since checkForInactiveClients function already does everything for us
        // {
        //	process.dead = true;
        //	log.debug() << "Calling " << func("waitpid") << punct("()") << " on pid " << repr(process.pid) << " with flags " <<
        // num("WNOHANG") << std::endl; 	int status; 	(void)::waitpid(process.pid, &status, WNOHANG);
        // }
        process.done = true;

        if (!process.headersSent) {
            log.warning() << "CGI process " << repr(process)
                          << " didn't send any data (not even headers), sending 502 Bad "
                             "Gateway to client"
                          << std::endl;
            sendError(clientSocket, 502, process.location);
        } else if (!process.response.empty()) {
            log.debug() << "CGI process is done, queueing remaining data" << std::endl;
            log.trace() << "The data is: " << repr(process.response) << std::endl;
            queueWrite(clientSocket, process.response);
        }

        log.debug() << "Removing remote client " << repr(clientSocket) << " from persistent connections" << std::endl;
        persistConns.erase(clientSocket);
        closeAndRemoveMultPlexFd(_monitorFds, cgiFd);
        log.debug() << "Removing remote client " << repr(clientSocket) << " from clientToCgi map" << std::endl;
        _clientToCgi.erase(clientSocket);
        log.debug() << "Removing CGI pipe FD " << repr(cgiFd) << " from cgiToClient map" << std::endl;
        _cgiToClient.erase(cgiFd);
        return;
    }

    process.lastActive = std::time(NULL);
    log.debug() << "Updating lastActive time for CGI process " << repr(process) << " to " << Utils::formattedTimestamp(process.lastActive)
                << std::endl;
    buffer[bytesRead] = '\0';
    process.totalSize += static_cast<unsigned long>(bytesRead);

    log.debug() << "Total Response size from CGI process " << repr(process) << " is " << repr(process.totalSize) << std::endl;
    log.trace() << "The raw data read from the CGI pipe FD: " << repr(string(buffer, static_cast<size_t>(bytesRead))) << std::endl;
    if (process.totalSize > Constants::cgiMaxResponseBodySize) { // 10GiB limit // TODO: why put a limit?
        log.warning() << "CGI Response size exceeded limit of 10GiB, namely " << repr(process.totalSize) << " (roughly "
                      << Utils::formatSI(process.totalSize) << ")" << std::endl;
        log.debug() << "Sending " << num("SIGKILL") << " using " << func("kill") << punct("()") << " to the CGI process with PID "
                    << repr(process.pid) << std::endl;
        (void)::kill(process.pid, SIGKILL);
        sendError(clientSocket, 413,
                  process.location); // TODO: @timo: Use macros for status codes instead
                                     // of magic numbers
        closeAndRemoveMultPlexFd(_monitorFds, cgiFd);
        log.debug() << "Removing remote client " << repr(clientSocket) << " from clientToCgi map" << std::endl;
        _clientToCgi.erase(clientSocket);
        log.debug() << "Removing CGI pipe FD " << repr(cgiFd) << " from cgiToClient map" << std::endl;
        _cgiToClient.erase(cgiFd);
        return;
    }

    if (!process.headersSent) {
        log.debug() << "Buffering data from CGI stdout (headers have NOT yet been sent): "
                    << repr(const_cast<char *>(string(buffer, static_cast<size_t>(bytesRead)).c_str())) << std::endl;
        process.response.append(buffer, static_cast<size_t>(bytesRead));
        size_t headerEnd = process.response.find("\r\n\r\n");
        if (headerEnd != string::npos) {
            log.debug() << "Found end of header fields marker, preparing response" << std::endl;
            // Found headers, prepare full response with HTTP/1.1
            string headers = process.response.substr(0, headerEnd);
            string body = process.response.substr(headerEnd + 4);

            std::ostringstream fullResponse;
            fullResponse << "HTTP/1.1 200 OK\r\n";

            // Add Content-Type if not present
            if (Utils::strToLower(headers).find("content-type:") == string::npos)
                fullResponse << "Content-Type: text/html\r\n";

            fullResponse << headers << "\r\n\r\n" << body;

            process.headersSent = true;
            queueWrite(clientSocket, fullResponse.str());
            log.debug() << "Clearing response buffer" << std::endl;
            process.response.clear();

        } else if (process.response.length() > Constants::cgiMaxResponseSizeWithoutBody) { // Status line + Headers
                                                                                           // too long
            log.warning() << "CGI Response size exceeded limit of 10GiB, namely " << repr(process.totalSize) << " (roughly "
                          << Utils::formatSI(process.totalSize) << ")" << std::endl;
            log.debug() << "Sending " << num("SIGKILL") << " using " << func("kill") << punct("()") << " to the CGI process with PID "
                        << repr(process.pid) << std::endl;
            (void)::kill(process.pid, SIGKILL);
            sendError(clientSocket, 502, process.location);
            // TODO: @all: not sure if the following line is correct
            closeAndRemoveMultPlexFd(_monitorFds, cgiFd);
            log.debug() << "Removing remote client " << repr(clientSocket) << " from clientToCgi map" << std::endl;
            _clientToCgi.erase(clientSocket);
            log.debug() << "Removing CGI pipe FD " << repr(cgiFd) << " from cgiToClient map" << std::endl;
            _cgiToClient.erase(cgiFd);
            return;
        } else {
            log.debug() << "End of header fields marker not yet found, buffering data further" << std::endl;
        }
    } else {
        log.debug() << "Queueing data for sending from CGI stdout (headers are already sent): "
                    << repr(const_cast<char *>(string(buffer, static_cast<size_t>(bytesRead)).c_str())) << std::endl;
        queueWrite(clientSocket, string(buffer, static_cast<size_t>(bytesRead)));
    }
}

bool HttpServer::parseRequestLine(int clientSocket, const string &_line, HttpRequest &request) {
    string line = _line;
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    line.erase(line.find_last_not_of(" \t\r\n") + 1);
    log.trace() << "Trimmed original line " << repr(_line) << " to this: " << repr(line) << std::endl;
    log.debug() << "Parsing request line: " << repr(line) << std::endl;
    if (line.length() > Constants::clientMaxRequestSizeWithoutBody) {
        log.debug() << "Request line is too large: " << repr(line) << std::endl;
        // should only the request.path be checked? Or the entire request line? who knows
        sendError(clientSocket, 414, NULL);
        return false;
    }
    std::istringstream iss(line);
    if (!(iss >> request.method >> request.path >> request.httpVersion)) { // NOTODO: @all: technically, only space is allowed as a
                                                                           // separator, not just any whitespace
        log.debug() << "Failed to parse request line: " << repr(line) << std::endl;
        sendError(clientSocket, 400, NULL);
        return false;
    }

    if (iss.rdbuf()->in_avail()) { // this is to check that there are no additional fields
        log.debug() << "Failed to parse request line (too many fields): " << repr(line) << std::endl;
        sendError(clientSocket, 400, NULL);
        return false;
    }
    size_t idxOfQuestionMark = request.path.find('?');
    if (idxOfQuestionMark != request.path.npos)
        request.rawQuery = request.path.substr(idxOfQuestionMark + 1);
    request.path = request.path.substr(0, idxOfQuestionMark);
    request.path = canonicalizePath(request.path);
    request.pathParsed = true;
    log.debug() << "Successfully parsed request line." << std::endl;
    log.debug() << "Method is " << repr(request.method) << std::endl;
    log.debug() << "(Canonicalized) path is " << repr(request.path) << std::endl;
    log.debug() << "Raw query is " << repr(request.rawQuery) << std::endl;
    log.debug() << "HTTP Version is " << repr(request.httpVersion) << std::endl;
    return true;
}

bool HttpServer::parseHeader(const string &line, HttpRequest &request) {
    // not doing obs-fold, aka line folding (rfc2616), since it has been obsoleted in
    // rfc7230.
    // single header == single line (ends in \r\n)
    // https://stackoverflow.com/questions/31237198/is-it-possible-to-include-multiple-crlfs-in-a-http-header-field
    size_t colonPos = line.find(':');
    if (colonPos == string::npos) {
        log.warning() << "Failed to parse header (missing colon): " << repr(line) << std::endl;
        return false;
    }

    // https://www.rfc-editor.org/rfc/rfc2616#section-4.2
    // "Field names [==header names, keys, whatever] are case-insensitive."
    // Therefore, always index using a lowercase string!
    string key = Utils::strToLower(line.substr(0, colonPos));
    string value = line.substr(colonPos + 1);

    // Trim leading/trailing linear whitespace
    value.erase(0, value.find_first_not_of(" \t\r\n"));
    value.erase(value.find_last_not_of(" \t\r\n") + 1);

    log.debug() << "Successfully parsed header." << std::endl;
    log.debug() << "(Lowercased) Name: " << repr(key) << std::endl;
    log.debug() << "Field Value: " << repr(value) << std::endl;
    request.headers[key] = value;
    return true;
}

void HttpServer::handleDelete(int clientSocket, const HttpRequest &request, const LocationCtx &location) {
    log.debug() << "Trying to handle delete for location " << repr(location) << std::endl;
    if (getFirstDirective(location.second, "upload_dir")[0].empty()) {
        log.debug() << "upload_dir directive is empty, therefore file deletion is "
                       "forbidden on this location, sending 405 Method Not Allowed"
                    << std::endl;
        sendError(clientSocket, 405, NULL);
    }
    string diskPath = determineDiskPath(request, location);

    struct stat fileStat;
    int fileExists = stat(diskPath.c_str(), &fileStat) == 0;

    if (!fileExists) {
        log.debug() << "File " << repr(diskPath) << " does not exist and therefore cannot be deleted, sending 404 Not Found" << std::endl;
        sendError(clientSocket, 404, &location);
    } else if (S_ISDIR(fileStat.st_mode)) {
        log.debug() << "File " << repr(diskPath) << " is a directory and directory deletion is not supported" << std::endl;
        sendString(clientSocket, "Directory deletion is turned off\n", 403);
    } else if (S_ISREG(fileStat.st_mode)) {
        log.debug() << "File " << repr(diskPath) << " is a regular file, deleting it" << std::endl;
        if (std::remove(diskPath.c_str()) < 0) {
            log.warning() << "File " << repr(diskPath) << " could not be deleted" << std::endl;
            sendString(clientSocket, "Failed to delete resource " + request.path + "\n", 500);
        } else {
            log.debug() << "File " << repr(diskPath) << " was successfully deleted" << std::endl;
            sendString(clientSocket, "Successfully deleted " + request.path + "\n");
        }
    } else {
        log.debug() << "File " << repr(diskPath) << " is neither a directory nor a regular file, sending 403 Forbidden" << std::endl;
        sendString(clientSocket, "You may only delete regular files\n", 403);
    }
}

static string getFileName(const string &path) {
    Logger::lastInstance().debug() << "Getting file name from path " << repr(path) << std::endl;
    size_t pos;
    if ((pos = path.rfind('/')) == path.npos) {
        Logger::lastInstance().debug() << "Name is " << repr(path) << std::endl;
        return path;
    }
    Logger::lastInstance().debug() << "Name is " << repr(path.substr(pos)) << std::endl;
    return path.substr(pos);
}

void HttpServer::handleUpload(int clientSocket, const HttpRequest &request, const LocationCtx &location, bool overwrite) {
    log.debug() << "Trying to handle file upload for location " << repr(location) << std::endl;
    if (getFirstDirective(location.second, "upload_dir")[0].empty()) {
        log.debug() << "upload_dir directive is empty, therefore file upload is "
                       "forbidden on this location, sending 405 Method Not Allowed"
                    << std::endl;
        sendError(clientSocket, 405, NULL);
    }
    log.debug() << "upload_dir directive exists, getting file name" << std::endl;
    string fileName = getFileName(request.path);
    string diskPath = getFirstDirective(location.second, "root")[0] + getFirstDirective(location.second, "upload_dir")[0] + fileName;

    struct stat fileStat;
    int fileExists = stat(diskPath.c_str(), &fileStat) == 0;

    if (fileExists && !overwrite) {
        log.debug() << "File exists already and overwriting of files is turned off, "
                       "sending 409 Conflict"
                    << std::endl;
        sendError(clientSocket, 409, &location); // == conflict
    } else if (fileExists) {
        log.debug() << "File exists already but overwriting of files is turned on, file "
                       "will be overwritten"
                    << std::endl;
    }

    std::ofstream of(diskPath.c_str());
    if (!of) {
        log.error() << "Failed to open file on path " << repr(diskPath) << " for writing" << std::endl;
        sendError(clientSocket, 500, &location);
        return;
    }
    of.write(request.body.c_str(), static_cast<long>(request.body.length()));
    of.flush();
    of.close();
    log.debug() << "Written " << repr(request.body.length()) << " bytes to file " << repr(diskPath) << std::endl;
    sendString(clientSocket, "Successfully uploaded " + request.path + "\n", 201);
}

void HttpServer::handleRequestInternally(int clientSocket, const HttpRequest &request, const LocationCtx &location) {
    log.debug() << "Request is not for CGI, handling internally" << std::endl;
    if (request.method == "GET" || request.method == "HEAD")
        serveStaticContent(clientSocket, request, location);
    else if (request.method == "POST")
        handleUpload(clientSocket, request, location, false);
    else if (request.method == "PUT")
        handleUpload(clientSocket, request, location, true);
    else if (request.method == "DELETE")
        handleDelete(clientSocket, request, location);
    else if (request.method == "FTFT") {
        log.fatal() << "Easter egg method was requested" << std::endl;
        sendString(clientSocket, repr(*this) + "\n");
    } else {
        log.fatal() << "Method " << repr(request.method) << " is not implemented, sending 405 Method Not Allowed" << std::endl;
        sendError(clientSocket, 405, &location);
    }
}

// clang-format off
static bool methodIsImplemented(const string &method) {
    return false || 
		method == "GET"    ||
		method == "HEAD"   ||
		method == "POST"   ||
		method == "DELETE" ||
		method == "PUT"    ||
		method == "FTFT"   ||
		false;
}
// clang-format on

bool HttpServer::methodAllowed(int clientSocket, const HttpRequest &request, const LocationCtx &location) {
    if (!Utils::allUppercase(request.method)) {
        log.debug() << "Invalid request, method not all uppercase" << std::endl;
        sendError(clientSocket, 400, &location);
        return false;
    } else if (!directiveExists(location.second, "limit_except")) {
        if (methodIsImplemented(request.method)) {
            log.debug() << "Method " << repr(request.method)
                        << " is allowed since it's not forbidden using limit_except and "
                           "it is implemented"
                        << std::endl;
            return true;
        }
        log.debug() << "Method " << repr(request.method) << " is not allowed since it is not implemented" << std::endl;
        return false;
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
    sendError(clientSocket, 405, &location);
    return false;
}

void HttpServer::rewriteRequest(int clientSocket, int statusCode, const string &urlOrText, const LocationCtx &location) {
    log.debug() << "Rewriting request for client" << std::endl;
    if (DirectiveValidation::isDoubleQuoted(urlOrText))
        sendString(clientSocket, DirectiveValidation::decodeDoubleQuotedString(urlOrText), statusCode);
    else if (DirectiveValidation::isHttpUri(urlOrText))
        redirectClient(clientSocket, urlOrText, statusCode);
    else
        sendError(clientSocket, 500, &location); // proper parsing should catch this case!
}

void HttpServer::handleRequest(int clientSocket, const HttpRequest &request, const LocationCtx &location) {
    log.info() << "Handling request to path: " << repr(request.path) << " to matched location " << repr(location.first) << std::endl;
    log.trace() << "Request: " << repr(request) << std::endl;
    log.trace() << "Location: " << repr(location) << std::endl;
    if (!methodAllowed(clientSocket, request, location))
        return;
    else if (directiveExists(location.second, "return"))
        rewriteRequest(clientSocket, std::atoi(getFirstDirective(location.second, "return")[0].c_str()),
                       getFirstDirective(location.second, "return")[1], location);
    else if (!requestIsForCgi(request, location))
        handleRequestInternally(clientSocket, request, location);
    else
        try {
            log.debug() << "Trying to handle request with CGI" << std::endl;
            ArgResults cgiExts = getAllDirectives(location.second, "cgi_ext");
            for (ArgResults::const_iterator ext = cgiExts.begin(); ext != cgiExts.end(); ++ext) {
                string extension = (*ext)[0];
                string program = ext->size() > 1 ? (*ext)[1] : "";

                CgiHandler handler(*this, extension, program, log);
                log.debug() << "Checking if CgiHandler " << repr(handler) << " can handle request " << repr(request) << std::endl;
                if (handler.canHandle(request.path)) {
                    log.debug() << "CgiHandler " << repr(handler) << " can handle request with path " << repr(request.path) << std::endl;
                    handler.execute(clientSocket, request, location);
                    return;
                } else {
                    log.debug() << "CgiHandler " << repr(handler) << " cannot handle request with path " << repr(request.path) << std::endl;
                }
            }

            // No matching CGI handler found
            sendError(clientSocket, 404, &location);
        } catch (const std::exception &e) {
            log.error() << "CGI execution failed: " << ansi::red(e.what()) << std::endl;
            sendError(clientSocket, 500, &location);
        }
}

bool HttpServer::isHeaderComplete(const HttpRequest &request) const {
    return !request.method.empty() && !request.path.empty() && !request.httpVersion.empty();
}

void HttpServer::processContLenChunkedAndConnectionHeaders(int clientSocket, HttpRequest &request) {
    if (request.headers.find("content-length") != request.headers.end())
        request.contentLength = static_cast<size_t>(std::atoi(request.headers["content-length"].c_str()));

    if (request.headers.find("transfer-encoding") != request.headers.end() && request.headers["transfer-encoding"] == "chunked") {
        request.chunkParsingState = PARSE_CHUNK_SIZE;
        request.chunkedTransfer = true;
    }

    if (request.headers.find("connection") != request.headers.end() && request.headers["connection"] == "close") {
        log.debug() << "Requestion has 'Connection: close' header, marking the client socket " << repr(clientSocket) << " as non-persistent"
                    << std::endl;
        persistConns.erase(clientSocket);
    }
}

bool HttpServer::validateRequest(const HttpRequest &request, int clientSocket) {
    // First check headers are complete
    log.debug() << "Checking whether headers are complete" << std::endl;
    if (!isHeaderComplete(request)) {
        log.debug() << "Invalid request: Incomplete headers" << std::endl;
        sendError(clientSocket, 400, NULL);
        return false;
    }
    log.debug() << "Headers are complete" << std::endl;

    // NOT doing that here
    // log.debug() << "Checking if HTTP method is allowed" << std::endl;
    // // Check HTTP method
    // if (!methodIsImplemented(request.method)) { // NOTODO: @timo: make more stuff static
    //     log.debug() << "Invalid request: Unsupported method: " << repr(request.method) << std::endl;
    //     sendError(clientSocket, 405, NULL);
    //     return false;
    // }
    // log.debug() << "Method " << repr(request.method) << " is allowed" << std::endl;

    log.debug() << "Checking if HTTP version is allowed" << std::endl;
    // Check HTTP version
    if (request.httpVersion != "HTTP/1.1") {
        log.debug() << "Invalid request: Unsupported HTTP version: " << repr(request.httpVersion) << std::endl;
        sendError(clientSocket, 505, NULL);
        return false;
    }
    log.debug() << "Version " << repr(request.httpVersion) << " is allowed" << std::endl;

    // For POST requests, verify content info
    log.debug() << "Checking if POST/PUT request is sound" << std::endl;
    if ((request.method == "POST" || request.method == "PUT") && !request.chunkedTransfer && request.contentLength == 0) {
        log.debug() << "Invalid " << repr(request.method) << " request: No content length or chunked transfer" << std::endl;
        sendError(clientSocket, 400, NULL);
        return false;
    }
    log.debug() << "POST/PUT request is sound (content-length not " << repr(0) << " if not a chunked transfer)" << std::endl;

    log.debug() << "Request is good" << std::endl;
    return true;
}

size_t HttpServer::getRequestSizeLimit(int clientSocket, const HttpRequest &request) {
    try {
        log.trace() << "Trying to get request size limit" << std::endl;
        const LocationCtx &loc = requestToLocation(clientSocket, request);
        size_t limit = Utils::convertSizeToBytes(getFirstDirective(loc.second, "client_max_body_size")[0]);
        log.trace() << "Successfully got request size limit: " << repr(limit) << std::endl;
        return limit;
    } catch (const runtime_error &error) {
        log.error() << "Failed to determine client_max_body_size for client socket " << repr(clientSocket) << ", returning default of "
                    << Constants::defaultClientMaxBodySize << std::endl;
        return Utils::convertSizeToBytes(Constants::defaultClientMaxBodySize);
    }
}

void HttpServer::removeClientAndRequest(int clientSocket) {
    removeClient(clientSocket);
    log.debug() << "Removing socket " << repr(clientSocket) << " from pendingRequests" << std::endl;
    _pendingRequests.erase(clientSocket);
}

bool HttpServer::checkRequestBodySize(int clientSocket, const HttpRequest &request, size_t currentSize) {
    log.debug() << "Checking request body size limit" << std::endl;
    size_t sizeLimit = getRequestSizeLimit(clientSocket, request);
    if (currentSize > sizeLimit) {
        log.warning() << "Request body (so far) is too big: " << repr(currentSize) << " bytes but max allowed is " << repr(sizeLimit)
                      << std::endl;
        sendError(clientSocket, 413, NULL); // Payload Too Large
        log.debug() << "Removing socket " << repr(clientSocket) << " from pendingRequests" << std::endl;
        _pendingRequests.erase(clientSocket);
        return false;
    }
    log.trace() << "Request body size so far is " << repr(currentSize) << " which is OK (not bigger than " << repr(sizeLimit) << ")"
                << std::endl;
    return true;
}

bool HttpServer::checkRequestSizeWithoutBody(int clientSocket, size_t currentSize) {
    log.debug() << "Checking request without body (request line + headers) size limit" << std::endl;
    size_t sizeLimit = Constants::clientMaxRequestSizeWithoutBody;
    if (currentSize > sizeLimit) {
        log.warning() << "Request size (without body) (so far) is too big: " << repr(currentSize) << " bytes but max allowed is "
                      << repr(sizeLimit) << std::endl;
        sendError(clientSocket, 413, NULL); // Payload Too Large
        log.debug() << "Removing socket " << repr(clientSocket) << " from pendingRequests" << std::endl;
        _pendingRequests.erase(clientSocket);
        return false;
    }
    log.trace() << "Request size (without body) so far is " << repr(currentSize) << " which is OK (not bigger than " << repr(sizeLimit)
                << ")" << std::endl;
    return true;
}

bool HttpServer::getChunkSizeStrAndConsume(string &buffer, string &chunkSizeStr) {
    log.trace() << "Trying to read and consume chunk size specification from buffer: " << repr(buffer) << std::endl;
    vector<char> bytes;
    size_t i = 0;
    while (true) {
        char c = buffer[i];
        if (Utils::isHexDigitNoCase(c))
            bytes.push_back(c);
        else
            break;
        ++i;
    }
    if (i + 1 >= buffer.length()) {
        log.debug() << "Chunked request: Need more data" << std::endl;
        chunkSizeStr = "";
        return true; // need more data
    } else if (buffer[i] == '\r' && buffer[i + 1] == '\n') {
        log.debug() << "Successfully parsed chunk size specification" << std::endl;
        log.trace() << "Updating buffer from " << repr(buffer) << std::endl;
        log.trace() << "                  to " << repr(buffer.substr(i + 2)) << std::endl;
        buffer = buffer.substr(i + 2);
        chunkSizeStr = string(&bytes[0], bytes.size());
        return true; // success
    } else {
        log.warning() << "Error, chunk size ends in invalid bytes, expected \\r\\n, but got " << static_cast<int>(buffer[i]) << " and "
                      << static_cast<int>(buffer[i + 1]) << " (decimal)" << std::endl;
        return false; // error
    }
}

bool HttpServer::getChunkAndConsume(string &buffer, size_t chunkSize, string &retChunk) {
    log.debug() << "Trying to consume chunk of size " << repr(chunkSize) << std::endl;
    string chunk = buffer.substr(0, chunkSize);
    if (chunk.length() + 1 >= buffer.length()) {
        retChunk = "";
        return false; // need more data
    }
    log.debug() << "Chunk is " << repr(chunk) << " of size " << repr(chunk.length()) << std::endl;
    log.debug() << "Successfully parsed chunk size specification" << std::endl;
    log.trace() << "Updating buffer from " << repr(buffer) << std::endl;
    log.trace() << "                  to " << repr(buffer.substr(chunk.length() + 2)) << std::endl;
    buffer = buffer.substr(chunk.length() + 2);
    retChunk = chunk;
    return true;
}

bool HttpServer::processChunkedData(int clientSocket, HttpRequest &request) {
    if (request.chunkParsingState == PARSE_CHUNK_SIZE) {
        string chunkSizeStr;
        if (!getChunkSizeStrAndConsume(request.temporaryBuffer, chunkSizeStr)) {
            log.warning() << "Error parsing chunk size specification" << std::endl;
            return false;
        }
        if (chunkSizeStr.empty()) {
            log.debug() << "Couldn't parse entire chunk size specification, need more data" << std::endl;
            return true; // need more data, not updating request.body
        }
        request.chunkParsingState = PARSE_CHUNK;
        log.debug() << "Changed chunk parsing state to " << repr(request.chunkParsingState) << std::endl;
        request.thisChunkSize =
            Utils::hexToSize(chunkSizeStr); // saving parsed chunk size, since we'll need it when parsing the chunk itself
        if (!checkRequestBodySize(clientSocket, request, request.thisChunkSize + request.body.length())) {
            log.warning() << "Error while reading chunk: Anticipated chunk size of " << repr(request.thisChunkSize) << " is too big"
                          << std::endl;
            return false;
        }
    }

    string chunk;
    if (!getChunkAndConsume(request.temporaryBuffer, request.thisChunkSize, chunk)) {
        log.debug() << "Couldn't parse entire chunk, need more data" << std::endl;
        return true; // need more data, not updating request.body
    } else if (request.thisChunkSize == 0 && chunk.empty()) {
        request.state = REQUEST_COMPLETE;
        log.debug() << "Found last chunk, changed request state to " << repr(request.state) << std::endl;
        request.chunkParsingState = PARSE_CHUNK_SIZE;
        log.debug() << "Technically needless: Changed chunk parsing state to " << repr(request.chunkParsingState) << std::endl;
        return true; // end condition (and it was CRLF terminuated)
    } else if (request.thisChunkSize == 0 && !chunk.empty()) {
        log.warning() << "Error parsing chunk, chunk size of " << repr(0) << " was specified but a non-empty chunk followed" << std::endl;
        return false; // got data although chunksize was zero, invalid request
    } else if (request.thisChunkSize == chunk.length()) {
        log.debug() << "Successfully extracted one chunk from the temporary buffer, appending to request body" << std::endl;
        request.body += chunk;
        log.debug() << "Doing recursive chunk processing call, in case there are more chunks contained in the temporary buffer"
                    << std::endl;
        request.chunkParsingState = PARSE_CHUNK_SIZE;
        log.debug() << "Changed chunk parsing state to " << repr(request.chunkParsingState) << std::endl;
        return processChunkedData(clientSocket, request);
    } else {
        log.warning() << "Error parsing chunk, specified chunk size " << repr(request.thisChunkSize) << " is not actual chunk size "
                      << repr(chunk.length()) << std::endl;
        return false; // request.thisChunkSize is not equal to actual (parsed) chunk size, invalid request
    }
}

bool HttpServer::processRequestHeaders(int clientSocket, HttpRequest &request, const string &rawData) {
    log.debug() << "Processing request headers" << std::endl;
    size_t headerEnd = rawData.find("\r\n\r\n");
    log.debug() << "Looking for \\r\\n\\r\\n. Found?: " << repr(headerEnd != string::npos) << std::endl;
    if (headerEnd == string::npos) {
        log.debug() << "Not processing headers, waiting for more client data to arrive" << std::endl;
        return false; // Need more data
    }

    std::istringstream stream(rawData);
    string line;

    // Parse request line (GET /path HTTP/1.1)
    if (!std::getline(stream, line) || !parseRequestLine(clientSocket, line, request)) {
        _pendingRequests.erase(clientSocket);
        return false;
    }

    log.debug() << "Parsing all headers, line by line, \\n delimited" << std::endl;
    // Parse headers (Key: Value)
    while (std::getline(stream, line) && line != "\r") {
        log.debug() << "Got this raw header line: " << repr(line) << std::endl;
        if (!parseHeader(line, request)) {
            log.debug() << "Failed to parse header line: " << repr(line) << std::endl;
            sendError(clientSocket, 400, NULL);
            _pendingRequests.erase(clientSocket);
            return false;
        }
    }
    log.debug() << "Found line consisting only of \\r, finishing header parsing" << std::endl;

    // Process Content-Length and chunked transfer headers
    processContLenChunkedAndConnectionHeaders(clientSocket, request);
    log.debug() << "Content length: " << repr(request.contentLength) << " (if it is " << repr(0)
                << ", it can also mean the header not present, like with GET request)" << std::endl;
    log.debug() << "Chunked transfer: " << repr(request.chunkedTransfer) << std::endl;

    // Validate the request
    if (!validateRequest(request, clientSocket)) {
        log.debug() << "Request validation failed" << std::endl;
        log.debug() << "Removing socket " << repr(clientSocket) << " from pendingRequests" << std::endl;
        _pendingRequests.erase(clientSocket);
        return false;
    }

    // Move any remaining data to body
    if (headerEnd + 4 < rawData.length()) {
        if (request.chunkedTransfer) {
            request.temporaryBuffer = rawData.substr(headerEnd + 4);
            log.debug() << "Chunked request: Moved " << repr(request.temporaryBuffer.length()) << " bytes to temporary buffer" << std::endl;
            if (!processChunkedData(clientSocket, request)) { // this might set request.state to REQUEST_COMPLETE
                sendError(clientSocket, 400, NULL);
                _pendingRequests.erase(clientSocket);
                return false;
            }
            if (request.state == REQUEST_COMPLETE)
                return true;
        } else {
            request.body = rawData.substr(headerEnd + 4);
            request.bytesRead = request.body.length();
            log.debug() << "Unchunked request: Moved " << repr(request.body.length()) << " bytes to body" << std::endl;
        }
    } else {
        request.temporaryBuffer.clear();
        request.body.clear();
        request.bytesRead = 0;
    }

    // Update state
    request.state = READING_BODY;
    log.debug() << "Updating request parsing state to " << repr(request.state) << std::endl;

    // If no body expected, mark as complete
    if (!request.chunkedTransfer && (request.contentLength == 0 || request.bytesRead >= request.contentLength)) {
        request.state = REQUEST_COMPLETE;
        log.debug() << "No additional body data expected, updating request parsing state to " << repr(request.state) << std::endl;
    }

    return true;
}

bool HttpServer::processRequestBody(int clientSocket, HttpRequest &request, const char *buffer, size_t bytesRead) {

    if (request.chunkedTransfer) {
        log.debug() << "Processing chunked request body" << std::endl;
        log.debug() << "Appending new data (" << repr(bytesRead) << " bytes) to temporary buffer (currently "
                    << repr(request.temporaryBuffer.length()) << " bytes)" << std::endl;
        log.trace() << "Buffer is " << repr(const_cast<char *>(buffer)) << std::endl;
        log.trace() << "Temporary buffer is " << repr(request.temporaryBuffer) << std::endl;
        request.temporaryBuffer.append(buffer, bytesRead);
        if (!processChunkedData(clientSocket, request)) { // NOTE: this function might update the request state to REQUEST_COMPLETE
            sendError(clientSocket, 400, NULL);
            _pendingRequests.erase(clientSocket);
            return false;
        }
    } else {
        log.debug() << "Processing request body (unchunked)" << std::endl;
        // Append new data to body
        log.debug() << "Appending new data (" << repr(bytesRead) << " bytes) to body (currently " << repr(request.body.length())
                    << " bytes)" << std::endl;
        log.trace() << "Buffer is " << repr(const_cast<char *>(buffer)) << std::endl;
        log.trace() << "Body is " << repr(request.body) << std::endl;
        request.body.append(buffer, bytesRead);
        request.bytesRead += bytesRead;
    }

    // Check size limit
    if (!checkRequestBodySize(clientSocket, request, request.body.length()))
        return false;

    // Check if we have all the data
    if (!request.chunkedTransfer && request.bytesRead >= request.contentLength) {
        request.state = REQUEST_COMPLETE;
        log.debug() << "Done collecting request body (not chunked and bytes received "
                       "reached content-length), updating request parsing state to "
                    << repr(request.state) << std::endl;
    }

    return true;
}

void HttpServer::finalizeRequest(int clientSocket, HttpRequest &request) {
    log.debug() << "Match request to server/location" << std::endl;
    bool bound = false;
    try {
        const LocationCtx &location = requestToLocation(clientSocket, request);
        bound = true;
        log.debug() << "Found location " << repr(location) << " for request " << repr(request) << std::endl;
        handleRequest(clientSocket, request, location);
    } catch (const runtime_error &err) {
        if (!bound)
            throw; // requestToLocation may throw, but if handleRequest throws, we got
                   // bigger problems, thus, rethrowing
        log.error() << "Failed to match request to a server/location context: " << ansi::red(err.what()) << std::endl;
    }
    log.debug() << "Removing socket " << repr(clientSocket) << " from pendingRequests" << std::endl;
    _pendingRequests.erase(clientSocket);
}

void HttpServer::handleIncomingData(int clientSocket, const char *buffer, ssize_t bytesRead) {

    // TODO: @timo: 408, request timeout??
    log.debug() << "Received data length: " << repr(bytesRead) << std::endl;
    log.trace() << "The data is: " << repr(const_cast<char *>(buffer)) << std::endl;
    // Get or create request state
    HttpRequest &request = _pendingRequests[clientSocket];
    log.debug() << "Current request state: " << repr(request.state) << std::endl;

    // Process based on current state
    if (request.state == READING_HEADERS) {
        // During header reading, we temporarily accumulate data
        string rawData = request.temporaryBuffer + string(buffer, static_cast<size_t>(bytesRead));
        log.debug() << "Accumulated data length: " << repr(rawData.length()) << std::endl;

        // Check accumulated size (without body)
        if (!checkRequestSizeWithoutBody(clientSocket, rawData.length()))
            return;

        // Store for next iteration if headers aren't complete
        request.temporaryBuffer = rawData;

        // Try to process headers
        if (processRequestHeaders(clientSocket, request, rawData)) {
            log.debug() << "Headers fully processed. Body size so far: " << repr(request.body.length()) << std::endl;
            if (!request.chunkedTransfer)
                log.debug() << "Expected content length: " << repr(request.contentLength) << std::endl;
            else
                log.debug() << "Is chunked requests: " << repr(request.chunkedTransfer) << std::endl;
        }
    } else if (request.state == READING_BODY) {
        if (request.chunkedTransfer) {
            log.debug() << "Reading body using chunked transfer encoding. Current size: " << repr(request.bytesRead) << std::endl;
        } else {
            log.debug() << "Reading body (unchunked). Current size: " << repr(request.bytesRead)
                        << " Expected: " << repr(request.contentLength) << std::endl;
        }
        if (!processRequestBody(clientSocket, request, buffer, static_cast<size_t>(bytesRead))) {
            return;
        }
    }
    // no else if. if state was set to complete just now then might as well already handle
    // it in the same pass
    if (request.state == REQUEST_COMPLETE) { // If request is complete, handle it
        if (!checkRequestBodySize(clientSocket, request, request.body.length()))
            return;

        log.debug() << "Request completely received" << std::endl;
        finalizeRequest(clientSocket, request);
    }
}

void HttpServer::readFromClient(int clientSocket) {
    if (_cgiToClient.find(clientSocket) != _cgiToClient.end()) {
        log.debug() << "CGI process has written data to stdout, trying to read it from FD " << repr(clientSocket) << std::endl;
        handleCgiRead(clientSocket);
        return;
    }
    log.debug() << "FD " << repr(clientSocket) << " is a remote client socket, trying to read " << repr(CONSTANTS_CHUNK_SIZE)
                << " bytes from it" << std::endl;

    char buffer[CONSTANTS_CHUNK_SIZE + 1]; // for NUL termination
    log.debug() << "Calling " << func("recv") << punct("()") << " on FD " << repr(clientSocket)
                << " without any flags into a buffer of size " << repr(sizeof(buffer) - 1) << std::endl;
    errno = 0;
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    log.trace() << "errno: " << ::strerror(errno) << std::endl;

    log.debug() << "Actually read " << repr(bytesRead) << " bytes" << std::endl;
    if (bytesRead <= 0) {
        log.debug() << "Removing client (ignoring whether it's a persistent connection or not) since only " << repr(bytesRead)
                    << " bytes were read" << std::endl;
        persistConns.erase(clientSocket);
        removeClientAndRequest(clientSocket);
        _clientToCgi.erase(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0';
    log.debug() << "Handling incoming data" << std::endl;
    handleIncomingData(clientSocket, buffer, bytesRead);
}
