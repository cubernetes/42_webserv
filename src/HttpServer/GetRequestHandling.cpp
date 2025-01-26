#include "HttpServer.hpp"

bool HttpServer::handleDirectoryRedirect(int clientSocket, const string& uri) {
		if (uri[uri.length() - 1] != '/') {
			string redirectUri = uri + "/";
			std::ostringstream response;
			response << _httpVersionString << " " << 301 << " " << statusTextFromCode(301) << "\r\n"
					<< "Location: " << redirectUri << "\r\n"
					<< "Connection: close\r\n\r\n";
			string responseStr = response.str();
			send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
			removeClient(clientSocket);
			return true;
		}
		return false;
}

// TODO: @timo: precompute indexes and validate so that they don't contain slashes
bool HttpServer::handleIndexes(int clientSocket, const string& diskPath, const HttpRequest& request, const LocationCtx& location) {
	ArgResults res = getAllDirectives(location.second, "index");
	Arguments indexes;
	for (ArgResults::const_iterator args = res.begin(); args != res.end(); ++args) {
		indexes.insert(indexes.end(), args->begin(), args->end());
	}
	for (Arguments::const_iterator index = indexes.begin(); index != indexes.end(); ++index) {
		string indexPath = diskPath + *index;
		HttpRequest newRequest = request;
		newRequest.path += *index;
		if (handleUriWithoutSlash(clientSocket, indexPath, newRequest, false))
			return true;
	}
	return false;
}

void HttpServer::handleUriWithSlash(int clientSocket, const string& diskPath, const HttpRequest& request, const LocationCtx& location, bool sendErrorMsg) {
	struct stat fileStat;
	int fileExists = stat(diskPath.c_str(), &fileStat) == 0;

	if (!fileExists) {
		if (sendErrorMsg)
			sendError(clientSocket, 404);
		return;
	} else if (!S_ISDIR(fileStat.st_mode)) {
		if (sendErrorMsg)
			sendError(clientSocket, 404);
		return;
	} else {
		if (handleIndexes(clientSocket, diskPath, request, location))
			return;
		else if (getFirstDirective(location.second, "autoindex")[0] == "off") {
			if (sendErrorMsg)
				sendError(clientSocket, 403);
			return;
		}
		sendString(clientSocket, indexDirectory(request.path, diskPath));
		return;
	}
}

bool HttpServer::handleUriWithoutSlash(int clientSocket, const string& diskPath, const HttpRequest& request, bool sendErrorMsg) {
	struct stat fileStat;
	int fileExists = stat(diskPath.c_str(), &fileStat) == 0;

	if (!fileExists) {
		if (sendErrorMsg)
			sendError(clientSocket, 404);
		return false;
	} else if (S_ISDIR(fileStat.st_mode)) {
		if (!handleDirectoryRedirect(clientSocket, request.path)) {
			if (sendErrorMsg)
				sendError(clientSocket, 404);
			return false;
		}
		return true;
	} else if (S_ISREG(fileStat.st_mode)) {
		sendFileContent(clientSocket, diskPath);
		return true;
	}
	if (sendErrorMsg)
		sendError(clientSocket, 404); // sometimes it's also 403, or 500, but haven't figured out the pattern yet
	return false;
}

void HttpServer::serveStaticContent(int clientSocket, const HttpRequest& request, const LocationCtx& location) {
	string diskPath = getFirstDirective(location.second, "root")[0] + request.path;

	if (diskPath[diskPath.length() - 1] == '/')
		handleUriWithSlash(clientSocket, diskPath, request, location);
	else
		(void)handleUriWithoutSlash(clientSocket, diskPath, request);
}
