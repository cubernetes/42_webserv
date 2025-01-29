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
	const Arguments& allowedMethods = getFirstDirective(location.second, "limit_except");
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

void HttpServer::readFromClient(int clientSocket) {

	if (_cgiProcesses.find(clientSocket) != _cgiProcesses.end()) {
		handleCGIRead(clientSocket);
		return;
	}

	char buffer[CONSTANTS_CHUNK_SIZE];
	if (!recvToBuffer(clientSocket, buffer, CONSTANTS_CHUNK_SIZE))
		return;
	HttpRequest request = parseHttpRequest(buffer);
	
	bool bound = false; // hack courtesy of https://stackoverflow.com/a/43016806
	try {
		const LocationCtx& location = requestToLocation(clientSocket, request);
		bound = true;
		handleRequest(clientSocket, request, location);
	} catch (const runtime_error& err) {
		if (bound) throw; // runtime_error because of handleRequest
		// couldn't find a server, or location, or couldn't get sockname, which shouldn't be fatal so we return
		Logger::logError(err.what());
		return;
	}
}
