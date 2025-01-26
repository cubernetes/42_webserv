#include "HttpServer.hpp"

void HttpServer::queueWrite(int clientSocket, const string& data) {
	if (_pendingWrites.find(clientSocket) == _pendingWrites.end())
		_pendingWrites[clientSocket] = PendingWrite(data);
	else
		_pendingWrites[clientSocket].data += data;
	startMonitoringForWriteEvents(_monitorFds, clientSocket);
}

void HttpServer::terminatePendingCloses(int clientSocket) {
	if (_pendingCloses.find(clientSocket) != _pendingCloses.end()) {
		_pendingCloses.erase(clientSocket);
		removeClient(clientSocket);
	}
}

bool HttpServer::maybeTerminateConnection(PendingWrites::iterator it, int clientSocket) {
	if (it == _pendingWrites.end()) {
		// No pending writes, for POLL: remove POLLOUT from events
		stopMonitoringForWriteEvents(_monitorFds, clientSocket);
		// Also if this client's connection is pending to be closed, close it
		terminatePendingCloses(clientSocket);
		return true;
	}
	return false;
}

void HttpServer::terminateIfNoPendingData(PendingWrites::iterator& it, int clientSocket, ssize_t bytesSent) {
	PendingWrite& pw = it->second;
	pw.bytesSent += static_cast<size_t>(bytesSent);
	
	// Check whether we've sent everything
	if (pw.bytesSent >= pw.data.length()) {
		_pendingWrites.erase(it);
		// for POLL: remove POLLOUT from events since we're done writing
		stopMonitoringForWriteEvents(_monitorFds, clientSocket);
	}
}

void HttpServer::writeToClient(int clientSocket) {
	PendingWrites::iterator it = _pendingWrites.find(clientSocket);
	if (maybeTerminateConnection(it, clientSocket))
		return;

	PendingWrite& pw = it->second;
	const char* data = pw.data.c_str() + pw.bytesSent;
	size_t remaining = std::min(Constants::chunkSize, pw.data.length() - pw.bytesSent);
	
	ssize_t bytesSent = send(clientSocket, data, remaining, 0);
	if (bytesSent < 0) {
		removeClient(clientSocket);
		return;
	}
	
	terminateIfNoPendingData(it, clientSocket, bytesSent);
}

void HttpServer::sendFileContent(int clientSocket, const string& filePath) {
		std::ifstream file(filePath.c_str(), std::ifstream::binary);
		if (!file) {
			sendError(clientSocket, 403);
			return;
		}

		file.seekg(0, std::ios::end);
		long fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		std::ostringstream headers;
		headers << _httpVersionString << " " << 200 << " " << statusTextFromCode(200) << "\r\n"
				<< "Content-Length: " << fileSize << "\r\n"
				<< "Content-Type: " << getMimeType(filePath) << "\r\n"
				<< "Connection: close\r\n\r\n";

		queueWrite(clientSocket, headers.str());

		char buffer[CONSTANTS_CHUNK_SIZE];
		while(file.read(buffer, sizeof(buffer)))
			queueWrite(clientSocket, string(buffer, static_cast<size_t>(file.gcount())));
		if (file.gcount() > 0)
			queueWrite(clientSocket, string(buffer, static_cast<size_t>(file.gcount())));
		_pendingCloses.insert(clientSocket);
}

string HttpServer::wrapInHtmlBody(const string& text) {
	return "<html>\r\n\t<body>\r\n\t\t" + text + "\r\n\t</body>\r\n</html>\r\n";
}

void HttpServer::sendError(int clientSocket, int statusCode) {
	sendString(clientSocket, wrapInHtmlBody(
		"<h1>\r\n\t\t\t" + STR(statusCode) + " " + statusTextFromCode(statusCode) + "\r\n\t\t</h1>"
	), statusCode);
}

void HttpServer::sendString(int clientSocket, const string& payload, int statusCode, const string& contentType) {
	std::ostringstream	response;
	
	response << _httpVersionString << " " << statusCode << " " << statusTextFromCode(statusCode) << "\r\n"
			<< "Content-Type: " << contentType << "\r\n"
			<< "Content-Length: " << payload.length() << "\r\n"
			<< "Connection: close\r\n"
			<< "\r\n"
			<< payload;

	queueWrite(clientSocket, response.str());
	_pendingCloses.insert(clientSocket);
}
