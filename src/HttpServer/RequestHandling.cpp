#include "Config.hpp"
#include "HttpServer.hpp"
#include "CgiHandler.hpp"
#include <cstdio>
#include "DirectiveValidation.hpp"

bool HttpServer::requestIsForCgi(const HttpRequest& request, const LocationCtx& location) {
	if (!directiveExists(location.second, "cgi_dir"))
		return false;
	string uri = request.path;
	string cgi_dir = getFirstDirective(location.second, "cgi_dir")[0];
	if (!Utils::isPrefix(cgi_dir, uri))
		return false;
	return true;
}

void HttpServer::handleCGIRead(int fd) {

	std::map<int, CGIProcess>::iterator it = _cgiProcesses.find(fd);
	if (it == _cgiProcesses.end()) {
		// CGI process is not found
		sendError(fd, 502, NULL);
		return;
	}

	CGIProcess& process = it->second;
	char buffer[CONSTANTS_CHUNK_SIZE];

	ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
	if (bytesRead < 0) {
		kill(process.pid, SIGKILL);
		sendError(process.clientSocket, 502, process.location);
		closeAndRemoveMultPlexFd(_monitorFds, fd);
		_cgiProcesses.erase(fd);
		return;
	}

	if (bytesRead == 0) { // EOF - CGI process finished writing
		int status;
		waitpid(process.pid, &status, WNOHANG);

		if (!process.headersSent) 
			sendError(process.clientSocket, 502, process.location);
		else if (!process.response.empty())
			queueWrite(process.clientSocket, process.response);

		closeAndRemoveMultPlexFd(_monitorFds, fd);
		_cgiProcesses.erase(fd);
		return;
	}

	buffer[bytesRead] = '\0';
	process.totalSize += static_cast<unsigned long>(bytesRead);

	if (process.totalSize > 10 * 1024 * 1024) { // 10MB limit
		kill(process.pid, SIGKILL);
		sendError(process.clientSocket, 413, process.location);
		closeAndRemoveMultPlexFd(_monitorFds, fd);
		_cgiProcesses.erase(fd);
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
			kill(process.pid, SIGKILL);
			sendError(process.clientSocket, 502, process.location);
			closeAndRemoveMultPlexFd(_monitorFds, fd);
			_cgiProcesses.erase(fd);
			return;
		}
	} else {
		queueWrite(process.clientSocket, string(buffer, static_cast<size_t>(bytesRead)));
	}
}

bool HttpServer::parseRequestLine(const string& line, HttpRequest& request) {
	std::istringstream iss(line);
	if (!(iss >> request.method >> request.path >> request.httpVersion)) {
		return false;
	}
	
	request.path = canonicalizePath(request.path);
	return true;
}

bool HttpServer::parseHeader(const string& line, HttpRequest& request) {
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

void HttpServer::parseHeaders(std::istringstream& requestStream, string& line, HttpRequest& request) {
	while (std::getline(requestStream, line) && line != "\r") { // TODO: @all: headers might be very hard to parse (multiline, etc) // RFC should really be the reference
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
	requestLine	>> request.method >> request.path >> request.httpVersion;
	request.path = canonicalizePath(request.path);

	parseHeaders(requestStream, line, request);
	return request;
}

void HttpServer::handleDelete(int clientSocket, const HttpRequest& request, const LocationCtx& location) {
	if (!directiveExists(location.second, "upload_dir"))
		sendError(clientSocket, 405, NULL);
	string diskPath = determineDiskPath(request, location);

	struct stat fileStat;
	int fileExists = stat(diskPath.c_str(), &fileStat) == 0;

	if (!fileExists) {
		sendError(clientSocket, 404, &location);
	} else if (S_ISDIR(fileStat.st_mode)) {
		sendError(clientSocket, 403, &location);
	} else if (S_ISREG(fileStat.st_mode)) {
		if (::remove(diskPath.c_str()) < 0)
			sendError(clientSocket, 500, &location);
		else
			sendString(clientSocket, "Successfully deleted " + request.path);
	} else {
		sendError(clientSocket, 403, &location);
	}
}

void HttpServer::handleRequestInternally(int clientSocket, const HttpRequest& request, const LocationCtx& location) {
	if (request.method == "GET")
		serveStaticContent(clientSocket, request, location);
	else if (request.method == "POST")
		sendString(clientSocket, "POST for file upload not implemented yet\r\n");
	else if (request.method == "DELETE")
		handleDelete(clientSocket, request, location);
	else if (request.method == "4242")
		sendString(clientSocket, repr(*this) + '\n');
	else {
		sendError(clientSocket, 405, &location);
	}
}

bool HttpServer::methodAllowed(const HttpRequest& request, const LocationCtx& location) {
	if (!Utils::allUppercase(request.method))
		return false;
	else if (!directiveExists(location.second, "limit_except"))
		return true;
	string limit_except = "limit_except";
	const Arguments& allowedMethods = getFirstDirective(location.second, limit_except);
	for (Arguments::const_iterator method = allowedMethods.begin(); method != allowedMethods.end(); ++method) {
		if (*method == request.method)
			return true;
	}
	return false;
}

void HttpServer::rewriteRequest(int clientSocket, int statusCode, const string& urlOrText, const LocationCtx& location) {
	if (DirectiveValidation::isDoubleQuoted(urlOrText))
		sendString(clientSocket, DirectiveValidation::decodeDoubleQuotedString(urlOrText), statusCode);
	else if (DirectiveValidation::isHttpUri(urlOrText))
		redirectClient(clientSocket, urlOrText, statusCode);
	else
		sendError(clientSocket, 500, &location); // proper parsing should catch this case!
}

// MAYBE: refactor
void HttpServer::handleRequest(int clientSocket, const HttpRequest& request, const LocationCtx& location) {
	if (!methodAllowed(request, location))
		sendError(clientSocket, 405, &location);
	else if (directiveExists(location.second, "return"))
		rewriteRequest(clientSocket,
			std::atoi(getFirstDirective(location.second, "return")[0].c_str()),
			getFirstDirective(location.second, "return")[1],
			location);
	else if (!requestIsForCgi(request, location))
		handleRequestInternally(clientSocket, request, location);
	else try {
		// Get CGI configuration TODO: @sonia do the executeDirectly case
		ArgResults cgiExts = getAllDirectives(location.second, "cgi_ext");
		for (ArgResults::const_iterator ext = cgiExts.begin(); ext != cgiExts.end(); ++ext) {
			string extension = (*ext)[0];
			string program = ext->size() > 1 ? (*ext)[1] : "/usr/bin/python3";
			
			CgiHandler handler(*this, extension, program);
			if (handler.canHandle(request.path)) {
				handler.execute(clientSocket, request, location);
				return;
			}
		}
		
		// No matching CGI handler found
		sendError(clientSocket, 404, &location);
	} catch (const std::exception& e) {
		Logger::logError(string("CGI execution failed: ") + e.what());
		sendError(clientSocket, 500, &location);
	}
}

ssize_t HttpServer::recvToBuffer(int clientSocket, char *buffer, size_t bufSiz) {
	ssize_t bytesRead = recv(clientSocket, buffer, bufSiz, 0);
	
	if (bytesRead <= 0) {
		if (bytesRead == 0)
			removeClient(clientSocket);
		else
			Logger::logError(string("recv failed: ") + strerror(errno));
		return 0;
	}
	
	buffer[bytesRead] = '\0';
	return bytesRead;
}


bool HttpServer::isHeaderComplete(const HttpRequest& request) const {
	return !request.method.empty() && !request.path.empty() && !request.httpVersion.empty();
}

bool HttpServer::needsMoreData(const HttpRequest& request) const {
	if (request.state == READING_HEADERS) 
		return true;
	if (request.state == READING_BODY) {
		if (request.chunkedTransfer) 
			return true; // TODO: @sonia Will be handled in follow-up PR
		return request.bytesRead < request.contentLength;
	}
	return false;
}

void HttpServer::processContentLength(HttpRequest& request) {
	if (request.headers.find("Content-Length") != request.headers.end())
		request.contentLength = static_cast<size_t>(std::atoi(request.headers["Content-Length"].c_str()));
	
	if (request.headers.find("Transfer-Encoding") != request.headers.end() && 
		request.headers["Transfer-Encoding"] == "chunked")
		request.chunkedTransfer = true;
}

bool HttpServer::validateRequest(const HttpRequest& request) const {
	// First check headers are complete
	if (!isHeaderComplete(request)) {
		Logger::logDebug("Invalid request: incomplete headers");
		return false;
	}
	
	// Check HTTP method
	if (request.method != "GET" && request.method != "POST" && request.method != "DELETE") {
		Logger::logDebug("Invalid request: unsupported method: " + request.method);
		return false;
	}
	
	// Check HTTP version
	if (request.httpVersion != "HTTP/1.1") {
		Logger::logDebug("Invalid request: unsupported HTTP version: " + request.httpVersion);
		return false;
	}
	
	// For POST requests, verify content info
	if (request.method == "POST" && !request.chunkedTransfer && request.contentLength == 0) {
		Logger::logDebug("Invalid POST request: no content length or chunked transfer");
		return false;
	}
	
	return true;
}

size_t HttpServer::getRequestSizeLimit(const HttpRequest& request, const LocationCtx* location) {
	if (location && directiveExists(location->second, "client_max_body_size")) {
		return Utils::convertSizeToBytes(getFirstDirective(location->second, "client_max_body_size")[0]);
	}
	
	try {
		const LocationCtx& loc = requestToLocation(-1, request);
		if (directiveExists(loc.second, "client_max_body_size")) {
			return Utils::convertSizeToBytes(getFirstDirective(loc.second, "client_max_body_size")[0]);
		}
	} catch (const std::exception&) {
		if (directiveExists(_config.first, "client_max_body_size")) {
			return Utils::convertSizeToBytes(getFirstDirective(_config.first, "client_max_body_size")[0]);
		}
	}
	
	// Default nginx
	return Utils::convertSizeToBytes("1m");
}

void HttpServer::removeClientAndRequest(int clientSocket) {
	removeClient(clientSocket);
	_pendingRequests.erase(clientSocket);
}

bool HttpServer::checkRequestSize(int clientSocket, const HttpRequest& request, size_t currentSize) {
	size_t sizeLimit = getRequestSizeLimit(request);
	if (currentSize > sizeLimit) {
		sendError(clientSocket, 413, NULL); // Payload Too Large
		removeClientAndRequest(clientSocket);
		return false;
	}
	return true;
}

bool HttpServer::processRequestHeaders(int clientSocket, HttpRequest& request, const string& rawData) {
	size_t headerEnd = rawData.find("\r\n\r\n");
	Logger::logDebug("Looking for headers end marker. Found: " + STR(headerEnd != string::npos));
	if (headerEnd == string::npos) {
		return false; // Need more data
	}

	std::istringstream stream(rawData);
	string line;

	// Parse request line (GET /path HTTP/1.1)
	if (!std::getline(stream, line) || !parseRequestLine(line, request)) {
		Logger::logDebug("Failed to parse request line: [" + line + "]");
		sendError(clientSocket, 400, NULL);
		removeClientAndRequest(clientSocket);
		return false;
	}
	Logger::logDebug("Parsed request line: " + request.method + " " + request.path);
	// Parse headers (Key: Value)
	while (std::getline(stream, line) && line != "\r") {
		if (!parseHeader(line, request)) {
			Logger::logDebug("Failed to parse header line: [" + line + "]");
			sendError(clientSocket, 400, NULL);
			removeClientAndRequest(clientSocket);
			return false;
		}
		Logger::logDebug("Parsed header line: [" + line + "]");
	}

	// Process Content-Length and chunked transfer headers
	processContentLength(request);
	Logger::logDebug("Content length detected: " + STR(request.contentLength));
	Logger::logDebug("Chunked transfer: " + STR(request.chunkedTransfer));


	// Validate the request
	if (!validateRequest(request)) {
		Logger::logDebug("Request validation failed");
		sendError(clientSocket, 405, NULL);
		return false;
	}

	// Move any remaining data to body
	if (headerEnd + 4 < rawData.size()) {
		request.body = rawData.substr(headerEnd + 4);
		request.bytesRead = request.body.size();
		Logger::logDebug("Moved " + STR(request.bytesRead) + " bytes to body");
	} else {
		request.body.clear();
		request.bytesRead = 0;
	}

	// Update state
	request.state = READING_BODY;

	// If no body expected, mark as complete
	if (!request.chunkedTransfer && 
		(request.contentLength == 0 || request.bytesRead >= request.contentLength)) {
		Logger::logDebug("No body expected, marking as complete");
		request.state = REQUEST_COMPLETE;
	}

	return true;
}

bool HttpServer::processRequestBody(int clientSocket, HttpRequest& request, const char* buffer, size_t bytesRead) {
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

void HttpServer::finalizeRequest(int clientSocket, HttpRequest& request) {
	try {
		const LocationCtx& location = requestToLocation(clientSocket, request);
		handleRequest(clientSocket, request, location);
	} catch (const runtime_error& err) {
		Logger::logError(err.what());
	}
	_pendingRequests.erase(clientSocket);
}

void HttpServer::handleIncomingData(int clientSocket, const char* buffer, ssize_t bytesRead) {

	Logger::logDebug("Received data length: " + STR(bytesRead));
	Logger::logDebug("Raw data: [" + string(buffer, bytesRead) + "]");
	// Get or create request state
	HttpRequest& request = _pendingRequests[clientSocket];
	Logger::logDebug("Current request state: " + STR(static_cast<int>(request.state)));

	// Process based on current state
	if (request.state == READING_HEADERS) {
		// During header reading, we temporarily accumulate data
		string rawData = request.temporaryBuffer + string(buffer, static_cast<size_t>(bytesRead));
		Logger::logDebug("Accumulated data length: " + STR(rawData.size()));

		// Check accumulated size
		if (!checkRequestSize(clientSocket, request, rawData.size()))
			return;

		// Store for next iteration if headers aren't complete
		request.temporaryBuffer = rawData;

		// Try to process headers
		if (processRequestHeaders(clientSocket, request, rawData)) {
			Logger::logDebug("Headers processed. Body size so far: " + STR(request.body.size()));
			Logger::logDebug("Expected content length: " + STR(request.contentLength));
			request.temporaryBuffer.clear();
		}
	}
	else if (request.state == READING_BODY) {
		Logger::logDebug("Reading body. Current size: " + STR(request.bytesRead) + 
						" Expected: " + STR(request.contentLength));
		if (!processRequestBody(clientSocket, request, buffer, static_cast<size_t>(bytesRead))) {
			return;
		}
	}

	// If request is complete, handle it
	if (request.state == REQUEST_COMPLETE) {
		Logger::logDebug("Request complete. Processing...");
		finalizeRequest(clientSocket, request);
	}

}

void HttpServer::readFromClient(int clientSocket) {
	if (_cgiProcesses.find(clientSocket) != _cgiProcesses.end()) {
		handleCGIRead(clientSocket);
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
