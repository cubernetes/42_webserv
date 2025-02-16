#include <cstddef>
#include <ostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>

#include "Config.hpp"
#include "DirectoryIndexer.hpp"
#include "HttpServer.hpp"
#include "Repr.hpp"

void HttpServer::redirectClient(int clientSocket, const string &newUri, int statusCode) {
    log.debug() << "Redirecting client " << repr(clientSocket) << " to new URI " << repr(newUri) << " with status code " << repr(statusCode)
                << std::endl;
    std::ostringstream response;
    string body = wrapInHtmlBody("<h1>\r\n\t\t\t" + Utils::STR(statusCode) + " " + statusTextFromCode(statusCode) + "\r\n\t\t</h1>");
    response << _httpVersionString << " " << statusCode << " " << statusTextFromCode(statusCode) << "\r\n"
             << "Location: " << newUri << "\r\n"
             << "Content-Length: " << body.length() << "\r\n"
             << "Connection: keep-alive\r\n\r\n"
             << body;
    string responseStr = response.str();
    log.debug() << "Sending redirection response immediately using " << func("send") << punct("()") << std::endl;
    log.trace() << "Full response (" << repr(responseStr.length()) << " bytes): " << repr(responseStr) << std::endl;
    ::send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
    // not checking for <= 0 since we remove the client anyways
    removeClient(clientSocket);
}

bool HttpServer::handleDirectoryRedirect(int clientSocket, const string &path) {
    if (path[path.length() - 1] != '/') {
        log.debug() << "Appending a slash '/' to the end of path " << repr(path) << " and then doing a redirect" << std::endl;
        redirectClient(clientSocket, path + "/");
        return true;
    }
    log.debug() << "Can't do a directory redirect on a path that already ends with a slash: " << repr(path) << std::endl;
    return false;
}

// NOTODO: @timo: precompute indexes
bool HttpServer::handleIndexes(int clientSocket, const string &diskPath, const HttpRequest &request, const LocationCtx &location) {
    ArgResults res = getAllDirectives(location.second, "index");
    Arguments indexes;
    for (ArgResults::const_iterator args = res.begin(); args != res.end(); ++args) {
        indexes.insert(indexes.end(), args->begin(), args->end());
    }
    log.debug() << "Iterating through the following indexes: " << repr(indexes) << std::endl;
    for (Arguments::const_iterator index = indexes.begin(); index != indexes.end(); ++index) {
        string indexPath = diskPath + *index;
        HttpRequest newRequest = request;
        newRequest.path += *index;
        log.debug() << "Trying to handle index " << repr(*index) << std::endl;
        if (handlePathWithoutSlash(clientSocket, indexPath, newRequest, location, false)) {
            log.debug() << "Index " << repr(*index) << " was successfully handled! Not trying the other indexes" << std::endl;
            return true;
        }
        log.debug() << "Index " << repr(*index) << " could not be handled, skipping" << std::endl;
    }
    log.debug() << "No index file seems to exist in the requested directory" << std::endl;
    return false;
}

void HttpServer::handlePathWithSlash(int clientSocket, const string &diskPath, const HttpRequest &request, const LocationCtx &location,
                                     bool sendErrorMsg) {
    struct stat fileStat;
    int fileExists = ::stat(diskPath.c_str(), &fileStat) == 0;

    log.debug() << "Handling disk path " << repr(diskPath) << " with trailing slash" << std::endl;
    if (!fileExists) {
        log.debug() << "Directory " << repr(diskPath) << " does not exist" << std::endl;
        if (sendErrorMsg)
            sendError(clientSocket, 404, &location);
        return;
    } else if (!S_ISDIR(fileStat.st_mode)) {
        log.debug() << "Directory " << repr(diskPath) << " is not actually a directory" << std::endl;
        if (sendErrorMsg)
            sendError(clientSocket, 404, &location);
        return;
    } else {
        log.debug() << "Trying to handle index file serving first for directory " << repr(diskPath) << std::endl;
        if (handleIndexes(clientSocket, diskPath, request, location)) {
            return;
        }
        log.debug() << "Couldn't find any index files, trying autoindex next if enabled" << std::endl;
        if (getFirstDirective(location.second, "autoindex")[0] == "off") {
            log.debug() << "Autoindexing is disabled, sending 403 Forbidden" << std::endl;
            if (sendErrorMsg)
                sendError(clientSocket, 403, &location);
            return;
        }
        log.debug() << "Autoindexing is enabled, indexing directory " << repr(diskPath) << std::endl;
        DirectoryIndexer di(log);
        sendString(clientSocket, di.indexDirectory(request.path, diskPath), 200, "text/html", request.method == "HEAD");
        return;
    }
}

bool HttpServer::handlePathWithoutSlash(int clientSocket, const string &diskPath, const HttpRequest &request, const LocationCtx &location,
                                        bool sendErrorMsg) {
    struct stat fileStat;
    int fileExists = ::stat(diskPath.c_str(), &fileStat) == 0;

    log.debug() << "Handling disk path " << repr(diskPath) << " without trailing slash" << std::endl;
    if (!fileExists) {
        log.debug() << "File " << repr(diskPath) << " does not exist" << std::endl;
        if (sendErrorMsg)
            sendError(clientSocket, 404, &location);
        return false;
    } else if (S_ISDIR(fileStat.st_mode)) {
        log.debug() << "File " << repr(diskPath) << " is a directory" << std::endl;
        if (diskPath[diskPath.length() - 1] != '/' && request.path[request.path.length() - 1] == '/') {
            handlePathWithSlash(clientSocket, diskPath + "/", request, location, sendErrorMsg);
            return true;
        }
        if (!handleDirectoryRedirect(clientSocket, request.path)) {
            log.debug() << "Failed to handle directory redirect, sending 404 Not Found" << std::endl;
            if (sendErrorMsg)
                sendError(clientSocket, 404, &location);
            return false;
        }
        log.debug() << "Directory redirect handling was successful" << std::endl;
        return true;
    } else if (S_ISREG(fileStat.st_mode)) {
        log.debug() << "File " << repr(diskPath) << " is a regular file" << std::endl;
        return sendFileContent(clientSocket, diskPath, location, 200, "", request.method == "HEAD");
    }
    log.debug() << "File " << repr(diskPath) << " is neither a directory nor a regular file, sending 403 Forbidden." << std::endl;
    if (sendErrorMsg)
        sendError(clientSocket, 403,
                  &location); // sometimes it's also 404, or 500, but haven't figured out
                              // the pattern yet
    return false;
}

string HttpServer::determineDiskPath(const HttpRequest &request, const LocationCtx &location) {
    log.debug() << "Determining file to server for request " << repr(request) << " to location " << repr(location) << std::endl;
    string pathWithoutRoot;
    if (directiveExists(location.second, "alias")) { // NOTODO: @timo: sanitziation?
        string alias = getFirstDirective(location.second, "alias")[0];
        log.debug() << "Alias directive is present and has the value " << repr(alias) << std::endl;
        string prefix = location.first;
        size_t prefixLen = prefix.length();
        string path = request.path;
        pathWithoutRoot = alias + path.substr(prefixLen);
        log.debug() << "Original path " << repr(path) << " after alias substitution is now " << repr(pathWithoutRoot) << std::endl;
    } else {
        log.debug() << "No alias directive present, using original path " << repr(request.path) << " as relative path to root" << std::endl;
        pathWithoutRoot = request.path;
    }
    string root = getFirstDirective(location.second, "root")[0];
    log.debug() << "root is " << repr(root) << std::endl;
    log.debug() << "relative path is " << repr(pathWithoutRoot) << std::endl;
    string diskPath = root + pathWithoutRoot;
    log.debug() << "Path to file is " << repr(diskPath) << std::endl;
    return diskPath;
}

void HttpServer::serveStaticContent(int clientSocket, const HttpRequest &request, const LocationCtx &location) {
    log.debug() << "Trying to serve static content for request " << repr(request) << std::endl;
    string diskPath = determineDiskPath(request, location);

    if (diskPath[diskPath.length() - 1] == '/') {
        log.debug() << "Path ends with slash: " << repr(diskPath) << std::endl;
        handlePathWithSlash(clientSocket, diskPath, request, location);
    } else {
        log.debug() << "Path does not end with slash: " << repr(diskPath) << std::endl;
        (void)handlePathWithoutSlash(clientSocket, diskPath, request, location);
    }
}
