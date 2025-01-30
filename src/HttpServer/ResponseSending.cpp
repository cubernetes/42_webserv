#include "HttpServer.hpp"

void HttpServer::queueWrite(int clientSocket, const string& data) {
	if (_pendingWrites.find(clientSocket) == _pendingWrites.end())
		_pendingWrites[clientSocket] = string(data);
	else
		_pendingWrites[clientSocket] += data;
	startMonitoringForWriteEvents(_monitorFds, clientSocket);
}

void HttpServer::terminatePendingCloses(int clientSocket) {
	if (_pendingCloses.find(clientSocket) != _pendingCloses.end()) {
		_pendingCloses.erase(clientSocket);
		removeClient(clientSocket);
	}
}

bool HttpServer::maybeTerminateConnection(PendingWriteMap::iterator it, int clientSocket) {
	if (it == _pendingWrites.end()) { // no pending writes anymore, maybe close
		if (_clientToCgi.find(clientSocket) == _clientToCgi.end()) { // STOP, make sure CGI doesn't want to write data to this client
			if (_cgiToClient.count(clientSocket) == 0) { // STOP, make sure clientSocket is not a writeFd to the CGI process
				// No pending writes, for POLL: remove POLLOUT from events
				// TODO: @timo: is this really needed? aren't we removing the client anyways?
				stopMonitoringForWriteEvents(_monitorFds, clientSocket);
				// Also if this client's connection is pending to be closed, close it
				terminatePendingCloses(clientSocket);
			}
		}
		return true;
	}
	return false;
}

void HttpServer::terminateIfNoPendingDataAndNoCgi(PendingWriteMap::iterator& it, int clientSocket, ssize_t bytesSent) {
	PendingWrite& pw = it->second;
	
	// Check whether we've sent everything and that there are no CGI processes
	if (_clientToCgi.find(clientSocket) == _clientToCgi.end() && (pw.length() == 0 || bytesSent == 0)) {
		_pendingWrites.erase(it);
		// TODO: @all: what about removing pendingCloses?
		// for POLL: remove POLLOUT from events since we're done writing
		stopMonitoringForWriteEvents(_monitorFds, clientSocket);
		// TODO: @all: yeah actually we really have to close the connection here somehow, removeCLient or smth
		removeClient(clientSocket); // REALLY NOT SURE ABOUT THIS ONE
		_pendingCloses.erase(clientSocket); // also not 100% sure about this one
	}
}

void HttpServer::writeToClient(int clientSocket) {
	PendingWriteMap::iterator it = _pendingWrites.find(clientSocket);
	if (maybeTerminateConnection(it, clientSocket))
		return;

	PendingWrite& pw = it->second;
	size_t dataSize = std::min(Constants::chunkSize, pw.length());
	string dataChunk = pw.substr(0, dataSize);
	pw = pw.substr(dataSize); // remove chunk (that will be send) from the beginning
	const char* data = dataChunk.c_str();
	
	ssize_t bytesSent;
	if (_cgiToClient.count(clientSocket)) // it's a writeFd for the CGI, can't use send
		bytesSent = write(clientSocket, data, dataSize);
	else
		bytesSent = send(clientSocket, data, dataSize, 0);
	if (bytesSent < 0) {
		// TODO: @all: remove pending closes? clear pending writes?
		removeClient(clientSocket);
		return;
	}
	
	terminateIfNoPendingDataAndNoCgi(it, clientSocket, bytesSent);
}

// note, be careful sending errors from this function, as it could lead to infinite recursion! (the error sending function uses this function)
bool HttpServer::sendFileContent(int clientSocket, const string& filePath, const LocationCtx& location, int statusCode, const string& contentType) {
		std::ifstream file(filePath.c_str(), std::ifstream::binary);
		if (!file) {
			sendError(clientSocket, 403, &location);
			return false;
		}

		file.seekg(0, std::ios::end);
		long fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		std::ostringstream headers;
		headers << _httpVersionString << " " << statusCode << " " << statusTextFromCode(statusCode) << "\r\n"
				<< "Content-Length: " << fileSize << "\r\n"
				<< "Content-Type: " << (contentType.empty() ? getMimeType(filePath) : contentType) << "\r\n"
				<< "Connection: close\r\n\r\n";

		queueWrite(clientSocket, headers.str());

		char buffer[CONSTANTS_CHUNK_SIZE];
		while(file.read(buffer, sizeof(buffer)))
			queueWrite(clientSocket, string(buffer, static_cast<size_t>(file.gcount())));
		if (file.gcount() > 0)
			queueWrite(clientSocket, string(buffer, static_cast<size_t>(file.gcount())));
		_pendingCloses.insert(clientSocket);
		return true;
}

string HttpServer::wrapInHtmlBody(const string& text) {
	return "<html>\r\n\t<body>\r\n\t\t" + text + "\r\n\t</body>\r\n</html>\r\n";
}

static string getErrorPagePath(int statusCode, const LocationCtx& location) {
	ArgResults allErrPageDirectives = getAllDirectives(location.second, "error_page");
	for (ArgResults::const_iterator errPages = allErrPageDirectives.begin(); errPages != allErrPageDirectives.end(); ++errPages) {
		Arguments::const_iterator code = errPages->begin(), before = --errPages->end();
		for(; code != before; ++code) {
			if (atoi(code->c_str()) == statusCode) {
				return errPages->back();
			}
		}
	}
	return "";
}

bool HttpServer::sendErrorPage(int clientSocket, int statusCode, const LocationCtx& location) {
	string errorPageLocation = getErrorPagePath(statusCode, location);
	if (errorPageLocation.empty())
		return false;
	string errorPagePath = getFirstDirective(location.second, "root")[0] + errorPageLocation;
	std::ifstream file(errorPagePath.c_str());
	if (!file)
		return false;
	return sendFileContent(clientSocket, errorPagePath, location, statusCode);
}

// pass NULL as location if errorPages should not be served
void HttpServer::sendError(int clientSocket, int statusCode, const LocationCtx *const location) {
	Logger::logDebug("Sending error response: " + STR(statusCode));
	if (location == NULL || !sendErrorPage(clientSocket, statusCode, *location)) {
		string errorContent = wrapInHtmlBody("<h1>\r\n\t\t\t" + STR(statusCode) + " " + statusTextFromCode(statusCode) + "\r\n\t\t</h1>");
		Logger::logDebug("Error content: " + errorContent);
		sendString(clientSocket, wrapInHtmlBody("<h1>\r\n\t\t\t" + STR(statusCode) + " " + statusTextFromCode(statusCode) + "\r\n\t\t</h1>"), statusCode);
	}
}

void HttpServer::sendString(int clientSocket, const string& payload, int statusCode, const string& contentType) {
	Logger::logDebug("Preparing to send string response with status " + STR(statusCode));
	std::ostringstream	response;
	
	response << _httpVersionString << " " << statusCode << " " << statusTextFromCode(statusCode) << "\r\n"
			<< "Content-Type: " << contentType << "\r\n"
			<< "Content-Length: " << payload.length() << "\r\n"
			<< "Connection: close\r\n"
			<< "\r\n"
			<< payload;

	Logger::logDebug("Queueing response: " + response.str());
	queueWrite(clientSocket, response.str());
	_pendingCloses.insert(clientSocket);
}
