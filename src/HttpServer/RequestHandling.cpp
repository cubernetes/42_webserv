#include "HttpServer.hpp"
#include "CgiHandler.hpp"

bool HttpServer::requestIsForCgi(const HttpRequest& request, const LocationCtx& location) {
	if (!directiveExists(location.second, "cgi_dir"))
		return false;
	string uri = request.path;
	string cgi_dir = getFirstDirective(location.second, "cgi_dir")[0];
	if (!Utils::isPrefix(cgi_dir, uri))
		return false;
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

void HttpServer::handleRequestInternally(int clientSocket, const HttpRequest& request, const LocationCtx& location) {
	if (request.method == "GET")
		serveStaticContent(clientSocket, request, location);
	else if (request.method == "POST")
		sendString(clientSocket, "POST for file upload not implemented yet\r\n");
	else if (request.method == "DELETE")
		sendString(clientSocket, "DELETE for file deletion not implemented yet\r\n");
	else if (request.method == "4242")
		sendString(clientSocket, repr(*this) + '\n');
	else {
		sendError(clientSocket, 405, &location);
	}
}

void HttpServer::handleRequest(int clientSocket, const HttpRequest& request, const LocationCtx& location) {
	if (!requestIsForCgi(request, location)) {
		handleRequestInternally(clientSocket, request, location);
		return;
	}
	try {
		// Get CGI configuration
		ArgResults cgiExts = getAllDirectives(location.second, "cgi_ext");
		for (ArgResults::const_iterator ext = cgiExts.begin(); ext != cgiExts.end(); ++ext) {
			string extension = (*ext)[0];
			string program = ext->size() > 1 ? (*ext)[1] : "/usr/bin/python3";
			
			CgiHandler handler(extension, program);
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
