#include <cstddef>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>

#include "Config.hpp"
#include "DirectoryIndexer.hpp"
#include "HttpServer.hpp"

void HttpServer::redirectClient(int clientSocket, const string &newUri, int statusCode) {
    std::ostringstream response;
    response << _httpVersionString << " " << statusCode << " " << statusTextFromCode(statusCode) << "\r\n"
             << "Location: " << newUri << "\r\n"
             << "Connection: close\r\n\r\n";
    string responseStr = response.str();
    send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
    // not checking for <= 0 since we remove the client anyways
    removeClient(clientSocket);
}

bool HttpServer::handleDirectoryRedirect(int clientSocket, const string &uri) {
    if (uri[uri.length() - 1] != '/') {
        redirectClient(clientSocket, uri + "/");
        return true;
    }
    return false;
}

// TODO: @timo: precompute indexes and validate so that they don't contain slashes
bool HttpServer::handleIndexes(int clientSocket, const string &diskPath, const HttpRequest &request,
                               const LocationCtx &location) {
    ArgResults res = getAllDirectives(location.second, "index");
    Arguments indexes;
    for (ArgResults::const_iterator args = res.begin(); args != res.end(); ++args) {
        indexes.insert(indexes.end(), args->begin(), args->end());
    }
    for (Arguments::const_iterator index = indexes.begin(); index != indexes.end(); ++index) {
        string indexPath = diskPath + *index;
        HttpRequest newRequest = request;
        newRequest.path += *index;
        if (handleUriWithoutSlash(clientSocket, indexPath, newRequest, location, false))
            return true;
    }
    return false;
}

void HttpServer::handleUriWithSlash(int clientSocket, const string &diskPath, const HttpRequest &request,
                                    const LocationCtx &location, bool sendErrorMsg) {
    struct stat fileStat;
    int fileExists = stat(diskPath.c_str(), &fileStat) == 0;

    if (!fileExists) {
        if (sendErrorMsg)
            sendError(clientSocket, 404, &location);
        return;
    } else if (!S_ISDIR(fileStat.st_mode)) {
        if (sendErrorMsg)
            sendError(clientSocket, 404, &location);
        return;
    } else {
        if (handleIndexes(clientSocket, diskPath, request, location))
            return;
        else if (getFirstDirective(location.second, "autoindex")[0] == "off") {
            if (sendErrorMsg)
                sendError(clientSocket, 403, &location);
            return;
        }
        DirectoryIndexer di(log);
        sendString(clientSocket, di.indexDirectory(request.path, diskPath), 200, "text/html", request.method == "HEAD");
        return;
    }
}

bool HttpServer::handleUriWithoutSlash(int clientSocket, const string &diskPath, const HttpRequest &request,
                                       const LocationCtx &location, bool sendErrorMsg) {
    struct stat fileStat;
    int fileExists = stat(diskPath.c_str(), &fileStat) == 0;

    if (!fileExists) {
        if (sendErrorMsg)
            sendError(clientSocket, 404, &location);
        return false;
    } else if (S_ISDIR(fileStat.st_mode)) {
        if (!handleDirectoryRedirect(clientSocket, request.path)) {
            if (sendErrorMsg)
                sendError(clientSocket, 404, &location);
            return false;
        }
        return true;
    } else if (S_ISREG(fileStat.st_mode)) {
        return sendFileContent(clientSocket, diskPath, location, 200, "", request.method == "HEAD");
    }
    if (sendErrorMsg)
        sendError(clientSocket, 403,
                  &location); // sometimes it's also 404, or 500, but haven't figured out the pattern yet
    return false;
}

string HttpServer::determineDiskPath(const HttpRequest &request, const LocationCtx &location) {
    string path;
    if (directiveExists(location.second, "alias")) { // TODO: @timo: sanitziation?
        string alias = getFirstDirective(location.second, "alias")[0];
        string prefix = location.first;
        size_t prefixLen = prefix.length();
        string uri = request.path;
        path = alias + uri.substr(prefixLen);
    } else
        path = request.path;
    return getFirstDirective(location.second, "root")[0] + path;
}

void HttpServer::serveStaticContent(int clientSocket, const HttpRequest &request, const LocationCtx &location) {
    string diskPath = determineDiskPath(request, location);

    if (diskPath[diskPath.length() - 1] == '/')
        handleUriWithSlash(clientSocket, diskPath, request, location);
    else
        (void)handleUriWithoutSlash(clientSocket, diskPath, request, location);
}
