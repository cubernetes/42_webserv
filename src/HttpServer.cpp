///// #include <iostream>
///// #include <ostream>
///// #include <cstdlib>
///// #include <fstream>
///// #include <sys/socket.h>
///// #include <unistd.h>
///// #include <errno.h>
///// #include <limits.h>
///// #include <sys/stat.h>
///// #include <netinet/in.h>
#ifndef _GNU_SOURCE
# define _GNU_SOURCE /* POLLRDHUP, see poll(2) */
#endif

#include <vector>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <ctype.h>

#include "HttpServer.hpp"
#include "Constants.hpp"
#include "Logger.hpp"
#include "Config.hpp"
#include "Utils.hpp"
#include "Repr.hpp"
#include "DirectoryIndexing.hpp"

using std::runtime_error;
using std::map;
using std::cout;
using std::vector;
using std::string;
using Constants::SELECT;
using Constants::POLL;
using Constants::EPOLL;
using Utils::STR;

HttpServer::~HttpServer() {
	TRACE_DTOR;
	closeAndRemoveAllMultPlexFd(_monitorFds);
}

HttpServer::HttpServer(const string& configPath) : _listeningSockets(), _monitorFds(Constants::defaultMultPlexType), _pollFds(_monitorFds.pollFds), _httpVersionString(Constants::httpVersionString), _rawConfig(readConfig(configPath)), _config(parseConfig(_rawConfig)), _mimeTypes(), _statusTexts(), _pendingWrites(), _pendingCloses(), _servers(), _defaultServers() {
	TRACE_ARG_CTOR(const string&, configPath);
	initStatusTexts(_statusTexts);
	initMimeTypes(_mimeTypes);
	setupServers(_config);
}

HttpServer::operator string() const {
	return ::repr(*this);
}

std::ostream& operator<<(std::ostream& os, const HttpServer& httpServer) {
	return os << static_cast<string>(httpServer);
}

string HttpServer::getServerIpStr(const ServerCtx& serverCtx) {
	const Arguments hostPort = getFirstDirective(serverCtx.first, "listen");
	return hostPort[0];
}

struct in_addr HttpServer::getServerIp(const ServerCtx& serverCtx) {
	const Arguments hostPort = getFirstDirective(serverCtx.first, "listen");

	struct addrinfo hints, *res;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(hostPort[0].c_str(), NULL, &hints, &res) != 0) {
		throw runtime_error("Invalid IP address for listen directive in serverCtx config");
	}
	struct in_addr ipv4 = ((struct sockaddr_in*)res->ai_addr)->sin_addr;
	freeaddrinfo(res);
	return ipv4;
}

in_port_t HttpServer::getServerPort(const ServerCtx& serverCtx) {
	const Arguments hostPort = getFirstDirective(serverCtx.first, "listen");
	return htons((in_port_t)std::atoi(hostPort[1].c_str()));
}

void HttpServer::setupServers(const Config& config) {
	for (ServerCtxs::const_iterator serverCtx = config.second.begin(); serverCtx != config.second.end(); ++serverCtx) {
		Server server(
			serverCtx->first,
			serverCtx->second,
			getFirstDirective(serverCtx->first, "server_name")
		);

		server.ip = getServerIp(*serverCtx);
		server.port = getServerPort(*serverCtx);

		setupListeningSocket(server);
		cout << cmt("Server with names ") << repr(server.serverNames) << cmt(" is listening on ") << num(getServerIpStr(*serverCtx)) << num(":") << repr(server.port) << '\n';
			
		_servers.push_back(server);

		AddrPort addr(server.ip, server.port);
		if (_defaultServers.find(addr) == _defaultServers.end())
			_defaultServers[addr] = _servers.size() - 1;
	}
}

int HttpServer::createTcpListenSocket() {
	int listeningSocket = socket(AF_INET, SOCK_STREAM, 0); // TODO: @discuss: removed SOCK_NONBLOCK since we're doing I/O multiplexing, not nonblocking I/O (see other TODO's for more details)
	if (listeningSocket < 0)
		throw runtime_error("Failed to create socket");
	
	int opt = 1;
	if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(listeningSocket);
		throw runtime_error("Failed to set socket options");
	}
	return listeningSocket;
}

void HttpServer::bindSocket(int listeningSocket, const Server& server) {
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = server.port;
	address.sin_addr = server.ip;
	
	if (bind(listeningSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
		close(listeningSocket);
		throw runtime_error(string("bind error: ") + strerror(errno));
	}
}

void HttpServer::listenSocket(int listeningSocket) {
	if (listen(listeningSocket, SOMAXCONN) < 0) {
		close(listeningSocket);
		throw runtime_error(string("listen error: ") + strerror(errno));
	}
}

void HttpServer::setupListeningSocket(const Server& server) {
	int listeningSocket = createTcpListenSocket();
	
	bindSocket(listeningSocket, server);
	listenSocket(listeningSocket);
	
	struct pollfd pfd;
	pfd.fd = listeningSocket;
	pfd.events = POLLIN;
	_monitorFds.pollFds.push_back(pfd);

	_listeningSockets.push_back(listeningSocket);
}

size_t HttpServer::findMatchingServer(const string& host, const struct in_addr& addr, in_port_t port) const {
	string requestedHost = host;
	size_t colonPos = requestedHost.rfind(':');
	if (colonPos != string::npos) 
		requestedHost = requestedHost.substr(0, colonPos);
	for (size_t i = 0; i < _servers.size(); ++i) {
		const Server& server = _servers[i];
		if (server.port == port && memcmp(&server.ip, &addr, sizeof(addr)) == 0) {
			for (size_t j = 0; j < server.serverNames.size(); ++j) {
				if (requestedHost == server.serverNames[i]) {
					return i + 1;
				}
			}
		}
	}

	AddrPort addrPort(addr, port);
	DefaultServers::const_iterator it = _defaultServers.find(addrPort);
	if (it != _defaultServers.end()) {
		return it->second + 1;
	}

	return 0;
}

void HttpServer::queueWrite(int clientSocket, const string& data) {
	if (_pendingWrites.find(clientSocket) == _pendingWrites.end())
		_pendingWrites[clientSocket] = PendingWrite(data);
	else
		_pendingWrites[clientSocket].data += data;
	startMonitoringForWriteEvents(_monitorFds, clientSocket);
}

void HttpServer::updatePollEvents(MultPlexFds& monitorFds, int clientSocket, short events, bool add) {
	for (size_t i = 0; i < monitorFds.pollFds.size(); ++i) {
		if (monitorFds.pollFds[i].fd == clientSocket) {
			if (add)
				monitorFds.pollFds[i].events |= events;
			else
				monitorFds.pollFds[i].events &= ~events;
			break;
		}
	}
}

void HttpServer::terminatePendingCloses(int clientSocket) {
	if (_pendingCloses.find(clientSocket) != _pendingCloses.end()) {
		_pendingCloses.erase(clientSocket);
		removeClient(clientSocket);
	}
}

void HttpServer::startMonitoringForWriteEvents(MultPlexFds& monitorFds, int clientSocket) {
	switch (monitorFds.multPlexType) {
		case SELECT:
			throw std::logic_error("Starting monitoring for write events for select type fds not implemented yet"); break;
		case POLL:
			updatePollEvents(monitorFds, clientSocket, POLLOUT, true); break;
		case EPOLL:
			throw std::logic_error("Starting monitoring for write events for epoll type fds not implemented yet"); break;
		default:
			throw std::logic_error("Starting monitoring for write events for unknown type fds not implemented yet");
	}
}

void HttpServer::stopMonitoringForWriteEvents(MultPlexFds& monitorFds, int clientSocket) {
	switch (monitorFds.multPlexType) {
		case SELECT:
			throw std::logic_error("Stopping monitoring for write events for select type fds not implemented yet"); break;
		case POLL:
			updatePollEvents(monitorFds, clientSocket, POLLOUT, false); break;
		case EPOLL:
			throw std::logic_error("Stopping monitoring for write events for epoll type fds not implemented yet"); break;
		default:
			throw std::logic_error("Stopping monitoring for write events for unknown type fds not implemented yet");
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
	size_t remaining = std::min(Constants::chunkSize, pw.data.length() - pw.bytesSent); // TODO: @discuss: we don't want to send 10GB responses, that'll block the server for everyone; TODO: @discuss: is `chunkSize` the correct term?
	
	ssize_t bytesSent = send(clientSocket, data, remaining, 0);
	if (bytesSent < 0) {
		removeClient(clientSocket);
		return;
	}
	
	terminateIfNoPendingData(it, clientSocket, bytesSent);
}

void HttpServer::addClientSocketToPollFds(MultPlexFds& monitorFds, int clientSocket) {
	struct pollfd pfd;
	pfd.fd = clientSocket;
	pfd.events = POLLIN | POLLOUT;
	pfd.revents = 0;
	monitorFds.pollFds.push_back(pfd);
}

void HttpServer::addClientSocketToMonitorFds(MultPlexFds& monitorFds, int clientSocket) {
	switch (monitorFds.multPlexType) {
		case SELECT:
			throw std::logic_error("Adding select type fds not implemented yet"); break;
		case POLL:
			addClientSocketToPollFds(monitorFds, clientSocket); break;
		case EPOLL:
			throw std::logic_error("Adding epoll type fds not implemented yet"); break;
		default:
			throw std::logic_error("Adding unknown type of fd not implemented");
	}
}

void HttpServer::addNewClient(int listeningSocket) {
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	
	int clientSocket = accept(listeningSocket, (struct sockaddr*)&clientAddr, &clientLen);
	if (clientSocket < 0) {
		// Not checking EWOULDBLOCK/EAGAIN, since we're not implmementing nonblocking IO, but the mutually exclusive I/O multiplexing model
		// subject forbids to check errno after and read or write, for this exact reason. accept is not mentioned, but it's the same idea
		Logger::logError(string("accept failed: ") + strerror(errno));
		return;
	}
	
	addClientSocketToMonitorFds(_monitorFds, clientSocket);
	Logger::logDebug("New client connected. FD: " + STR(clientSocket));
}

size_t HttpServer::findMatchingLocation(const Server& server, const string& path) const {
	int bestIdx = -1;
	int bestScore = -1;
	for (size_t i = 0; i < server.locations.size(); ++i) {
		LocationCtx loc = server.locations[i];
		const string& locPath = loc.first;
		pair<string::const_iterator, string::const_iterator> matcher = std::mismatch(locPath.begin(), locPath.end(), path.begin());
		int currentScore = static_cast<int>(std::distance(locPath.begin(), matcher.first));
		if (currentScore > bestScore) {
			bestScore = currentScore;
			bestIdx = static_cast<int>(i);
		}
	}
	if (bestIdx >= 0) {
		return bestIdx + 1;
	}

	// TODO: @all: there MUST be a default location on "/", like with nginx, otherwise complicated
	// location = server.locations[0];
	return 0;
}

void HttpServer::readFromClient(int clientSocket) {
	char buffer[CONSTANTS_CHUNK_SIZE];
	ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
	
	if (bytesRead <= 0) {
		if (bytesRead == 0)	// TODO: @all: quote subject: "checking the value of errno is strictly forbidden after a read or write operation"
							// recv/recvfrom and send/sendto are also read/write ops (just specialized for sockets).
							// setting the fd to NONBLOCK and then checking whether errno is EWOULDBLOCK or EAGAIN is called the "nonblocking IO" model, which is forbidden
							// namely, we have to implementing "I/O multiplexing". this video is superb: youtu.be/I5j9TBcqe_Q
			removeClient(clientSocket);
		else
			Logger::logError(string("recv failed: ") + strerror(errno));
		return;
	}
	
	buffer[bytesRead] = '\0';
	HttpRequest request = parseHttpRequest(buffer);

	struct sockaddr_in addr;
	socklen_t addrLen = sizeof(addr);
	if (getsockname(clientSocket, (struct sockaddr*)&addr, &addrLen) < 0) {
		sendError(clientSocket, 500);
		return;
	}
	
	// Get host from request headers
	string host;
	if (request.headers.find("Host") != request.headers.end()) {
		host = request.headers.at("Host");
		size_t colonPos = host.find(':');
		if (colonPos != string::npos)
			host = host.substr(0, colonPos);
	}

	size_t serverIdx;
	if (!(serverIdx = findMatchingServer(host, addr.sin_addr, addr.sin_port))) {
		sendError(clientSocket, 404); // TODO: @all: is 404 rlly correct?
		return;
	}
	const Server& server = _servers[serverIdx - 1]; // 0 indicates failure, 1 is the first server

	size_t locationIdx;
	if (!(locationIdx = findMatchingLocation(server, request.path))) {
		sendError(clientSocket, 404); // TODO: @all: is 404 rlly correct?
		return;
	}
	const LocationCtx& location = server.locations[locationIdx - 1];
	
	if (request.method == "GET")
		serveStaticContent(clientSocket, request, location);
	else {
		string response = _httpVersionString + " " + STR(501) + statusTextFromCode(501) + "\r\n"
					"Content-Length: 22\r\n"
					"Connection: close\r\n"
					"\r\n"
					"Method not implemented\n";

		send(clientSocket, response.c_str(), response.length(), 0);
		removeClient(clientSocket);
	}
}

HttpServer::HttpRequest HttpServer::parseHttpRequest(const char *buffer) {

	HttpRequest request;
	std::istringstream requestStream(buffer);
	string line;

	std::getline(requestStream, line);

	std::istringstream requestLine(line);
	requestLine	>> request.method >> request.path >> request.httpVersion;

	while (std::getline(requestStream, line) && line != "\r") {
		size_t colonPos = line.find(':');
		if (colonPos != string::npos) {
			string key = line.substr(0, colonPos);
			string value = line.substr(colonPos + 1);
			value.erase(0, value.find_first_not_of(" "));
			value.erase(value.find_last_not_of("\r") + 1);
			request.headers[key] = value;
		}
	}
	return request;
}

bool HttpServer::validatePath(int clientSocket, const string& path) {
		if (path.find("..") != string::npos || 
			path.find("//") != string::npos || 
			path[0] != '/' || 
			path.find(':') != string::npos) {
			sendError(clientSocket, 403);
			return false;
		}
		return true;
}

void HttpServer::removeClient(int clientSocket) {
	closeAndRemoveMultPlexFd(_monitorFds, clientSocket);
}

bool HttpServer::handleDirectoryRedirect(int clientSocket, const HttpRequest& request, string& filePath, 
								const string& defaultIndex, struct stat& fileStat) {
		if (request.path[request.path.length() - 1] != '/') {
			string redirectUrl = request.path + "/";
			std::ostringstream response;
			response << _httpVersionString << " " << 301 << statusTextFromCode(301) << "\r\n"
					<< "Location: " << redirectUrl << "\r\n"
					<< "Connection: close\r\n\r\n";
			string responseStr = response.str();
			send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
			removeClient(clientSocket);
			return false;
		}
		filePath += defaultIndex;
		if (stat(filePath.c_str(), &fileStat) == -1) {
			sendError(clientSocket, 404);
			return false;
		}
		return true;
}

void HttpServer::sendFileContent(int clientSocket, const string& filePath) {
		std::ifstream file(filePath.c_str(), std::ios::binary);
		if (!file) {
			sendError(clientSocket, 403);
			return;
		}

		file.seekg(0, std::ios::end);
		long fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		std::ostringstream headers;
		headers << _httpVersionString << " " << 200 << statusTextFromCode(200) << "\r\n"
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

void HttpServer::serveStaticContent(int clientSocket, const HttpRequest& request, const LocationCtx& location) {
	if (!validatePath(clientSocket, request.path))
		return;
	string rootDir = getFirstDirective(location.second, "root")[0];
	string defaultIndex = getFirstDirective(location.second, "index")[0];
	struct stat fileStat;
	string filePath = rootDir + request.path;
	int fileExists = stat(filePath.c_str(), &fileStat) == 0;

	if (filePath[filePath.length() - 1] == '/') {
		struct stat indexFileStat;
		string indexPath = filePath + defaultIndex;

		if (stat(indexPath.c_str(), &indexFileStat) == -1) {
			if (getFirstDirective(location.second, "autoindex")[0] == "off") {
				sendError(clientSocket, 403);
				return;
			}
			if (fileExists)
				sendString(clientSocket, indexDirectory(request.path, filePath));
			sendError(clientSocket, 404);
			return;
		}
		filePath = indexPath;
		fileExists = stat(filePath.c_str(), &fileStat) == 0;
	}

	if (!fileExists) {
		sendError(clientSocket, 404);
		return;
	}

	if (S_ISDIR(fileStat.st_mode)) {
		if (!handleDirectoryRedirect(clientSocket, request, filePath, defaultIndex, fileStat))
			return;
	}

	if (S_ISREG(fileStat.st_mode))
		sendFileContent(clientSocket, filePath);
	else
		sendError(clientSocket, 404); // sometimes it's also 403, or 500, but haven't figured out the pattern yet
}

void HttpServer::initMimeTypes(MimeTypes& mimeTypes) {

	// Web content
	mimeTypes["html"] = "text/html";
	mimeTypes["htm"] = "text/html";
	mimeTypes["css"] = "text/css";
	mimeTypes["js"] = "application/javascript";
	mimeTypes["xml"] = "application/xml";
	mimeTypes["json"] = "application/json";

	// Text files
	mimeTypes["txt"] = "text/plain";
	mimeTypes["csv"] = "text/csv";
	mimeTypes["md"] = "text/markdown";
	mimeTypes["sh"] = "text/x-shellscript";

	// Images
	mimeTypes["jpg"] = "image/jpeg";
	mimeTypes["jpeg"] = "image/jpeg";
	mimeTypes["png"] = "image/png";
	mimeTypes["gif"] = "image/gif";
	mimeTypes["svg"] = "image/svg+xml";
	mimeTypes["ico"] = "image/x-icon";
	mimeTypes["webp"] = "image/webp";

	// Documents
	mimeTypes["pdf"] = "application/pdf";
	mimeTypes["doc"] = "application/msword";
	mimeTypes["docx"] = "application/msword";
	mimeTypes["xls"] = "application/vnd.ms-excel";
	mimeTypes["xlsx"] = "application/vnd.ms-excel";
	mimeTypes["zip"] = "application/zip";

	// Multimedia
	mimeTypes["mp3"] = "audio/mpeg";
	mimeTypes["mp4"] = "video/mp4";
	mimeTypes["webm"] = "video/webm";

	// ...
}

///// bool HttpServer::setup(const Config& conf) {
///// 	_config = conf;
///// 	const Arguments hostPort = getFirstDirective(conf.second[0].first, "listen");
///// 	const char *host = hostPort[0].c_str();
///// 	int port = std::atoi(hostPort[1].c_str());
///// 	return setupSocket(host, port);
///// }
string HttpServer::statusTextFromCode(int statusCode) {
	if (_statusTexts.find(statusCode) == _statusTexts.end())
		return "Unknown Status Code";
	return _statusTexts[statusCode];
}

void HttpServer::initStatusTexts(StatusTexts& statusTexts) {
	statusTexts[100] = "Continue";
	statusTexts[101] = "Switching Protocols";
	statusTexts[102] = "Processing";
	statusTexts[103] = "Early Hints";
	statusTexts[200] = "OK";
	statusTexts[201] = "Created";
	statusTexts[202] = "Accepted";
	statusTexts[203] = "Non-Authoritative Information";
	statusTexts[204] = "No Content";
	statusTexts[205] = "Reset Content";
	statusTexts[206] = "Partial Content";
	statusTexts[207] = "Multi-Status";
	statusTexts[208] = "Already Reported";
	statusTexts[226] = "IM Used";
	statusTexts[300] = "Multiple Choices";
	statusTexts[301] = "Moved Permanently";
	statusTexts[302] = "Found";
	statusTexts[303] = "See Other";
	statusTexts[304] = "Not Modified";
	statusTexts[305] = "Use Proxy";
	statusTexts[306] = "Switch Proxy";
	statusTexts[307] = "Temporary Redirect";
	statusTexts[308] = "Permanent Redirect";
	statusTexts[404] = "error on Wikimedia";
	statusTexts[400] = "Bad Request";
	statusTexts[401] = "Unauthorized";
	statusTexts[402] = "Payment Required";
	statusTexts[403] = "Forbidden";
	statusTexts[404] = "Not Found";
	statusTexts[405] = "Method Not Allowed";
	statusTexts[406] = "Not Acceptable";
	statusTexts[407] = "Proxy Authentication Required";
	statusTexts[408] = "Request Timeout";
	statusTexts[409] = "Conflict";
	statusTexts[410] = "Gone";
	statusTexts[411] = "Length Required";
	statusTexts[412] = "Precondition Failed";
	statusTexts[413] = "Payload Too Large";
	statusTexts[414] = "URI Too Long";
	statusTexts[415] = "Unsupported Media Type";
	statusTexts[416] = "Range Not Satisfiable";
	statusTexts[417] = "Expectation Failed";
	statusTexts[418] = "I'm a teapot";
	statusTexts[421] = "Misdirected Request";
	statusTexts[422] = "Unprocessable Content";
	statusTexts[423] = "Locked";
	statusTexts[424] = "Failed Dependency";
	statusTexts[425] = "Too Early";
	statusTexts[426] = "Upgrade Required";
	statusTexts[428] = "Precondition Required";
	statusTexts[429] = "Too Many Requests";
	statusTexts[431] = "Request Header Fields Too Large";
	statusTexts[451] = "Unavailable For Legal Reasons";
	statusTexts[500] = "Internal Server Error";
	statusTexts[501] = "Not Implemented";
	statusTexts[502] = "Bad Gateway";
	statusTexts[503] = "Service Unavailable";
	statusTexts[504] = "Gateway Timeout";
	statusTexts[505] = "HTTP Version Not Supported";
	statusTexts[506] = "Variant Also Negotiates";
	statusTexts[507] = "Insufficient Storage";
	statusTexts[508] = "Loop Detected";
	statusTexts[510] = "Not Extended";
	statusTexts[511] = "Network Authentication Required";
}

string HttpServer::wrapInHtmlBody(const string& text) {
	return "<html><body>" + text + "</body></html>";
}

void HttpServer::sendError(int clientSocket, int statusCode) {
	sendString(clientSocket, wrapInHtmlBody(
		"<h1>" + STR(statusCode) + " " + statusTextFromCode(statusCode) + "</h1>"
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

string HttpServer::getMimeType(const string& path) {
	size_t dotPos = path.find_last_of('.');
	if (dotPos == string::npos)
		return Constants::defaultMimeType; // file without an extension
	
	string ext = path.substr(dotPos + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower); // a malicious MIME type could contain negative chars, leading to undefined behaviour with ::tolower, see https://stackoverflow.com/questions/5270780/what-does-the-mean-in-tolower
	MimeTypes::const_iterator it = _mimeTypes.find(ext);
	if (it != _mimeTypes.end())
		return it->second; // MIME type found
	return Constants::defaultMimeType; // MIME type not found
}

bool HttpServer::isListeningSocket(int fd) {
	return std::find(_listeningSockets.begin(), _listeningSockets.end(), fd) != _listeningSockets.end();
}

void HttpServer::handleReadyFds(const MultPlexFds& readyFds) {
	size_t nReadyFds = readyFds.fdStates.size();
	for (size_t i = 0; i < nReadyFds; ++i) {
		int fd = multPlexFdToRawFd(readyFds, i);
		if (readyFds.fdStates[i] == FD_READABLE) {
			if (isListeningSocket(fd))
				addNewClient(fd);
			else
				readFromClient(fd);
		} else if (readyFds.fdStates[i] == FD_WRITEABLE)
			writeToClient(fd);
		else
			throw std::logic_error("Cannot handle fd other than readable or writable at this step"); // TODO: @timo: proper logging
	}
}

struct pollfd *HttpServer::multPlexFdsToPollFds(const MultPlexFds& fds) {
	return (struct pollfd*)&fds.pollFds[0];
}

nfds_t HttpServer::getNumberOfPollFds(const MultPlexFds& fds) {
	return static_cast<nfds_t>(fds.pollFds.size());
}

void HttpServer::removePollFd(MultPlexFds& monitorFds, int fd) {
	for (PollFds::iterator pollFd = monitorFds.pollFds.begin(); pollFd != monitorFds.pollFds.end(); ++pollFd) {
		if (pollFd->fd == fd) {
			monitorFds.pollFds.erase(pollFd);
			break;
		}
	}
}

void HttpServer::closeAndRemoveAllPollFd(MultPlexFds& monitorFds) {
	for (PollFds::iterator pollFd = monitorFds.pollFds.begin(); pollFd != monitorFds.pollFds.end(); ++pollFd) {
		if (pollFd->fd >= 0) {
			close(pollFd->fd);
			pollFd->fd = -1;
		}
	}
	monitorFds.pollFds.clear();
}

void HttpServer::closeAndRemoveMultPlexFd(MultPlexFds& monitorFds, int fd) {
	close(fd); // TODO: @timo: guard every syscall
	switch (monitorFds.multPlexType) {
		case SELECT:
			throw std::logic_error("Removing select type fds not implemented yet"); break;
		case POLL:
			removePollFd(monitorFds, fd); break;
		case EPOLL:
			throw std::logic_error("Removing epoll type fds not implemented yet"); break;
		default:
			throw std::logic_error("Removing unknown type of fd not implemented");
	}
}

void HttpServer::closeAndRemoveAllMultPlexFd(MultPlexFds& monitorFds) {
	switch (monitorFds.multPlexType) {
		case SELECT:
			throw std::logic_error("Removing all select type fds not implemented yet"); break;
		case POLL:
			closeAndRemoveAllPollFd(monitorFds); break;
		case EPOLL:
			throw std::logic_error("Removing all epoll type fds not implemented yet"); break;
		default:
			throw std::logic_error("Removing all unknown types of fd not implemented");
	}
}

HttpServer::MultPlexFds HttpServer::getReadyPollFds(MultPlexFds& monitorFds, int nReady, struct pollfd *pollFds, nfds_t nPollFds) {
	MultPlexFds readyFds(POLL);

	(void)nReady;
	for (int i = 0; i < static_cast<int>(nPollFds); ++i) {
		if (pollFds[i].revents & POLLIN) {
			readyFds.pollFds.push_back(pollFds[i]);
			readyFds.fdStates.push_back(FD_READABLE);
		} else if (pollFds[i].revents & POLLOUT) {
			readyFds.pollFds.push_back(pollFds[i]);
			readyFds.fdStates.push_back(FD_WRITEABLE);
		// } else if (pollFds[i].revents & (POLLRDHUP | POLLERR | POLLHUP | POLLNVAL)) {
		} else if (pollFds[i].revents != 0) {
			closeAndRemoveMultPlexFd(monitorFds, pollFds[i].fd);
		}
	}

	return readyFds;
}

HttpServer::MultPlexFds HttpServer::doPoll(MultPlexFds& monitorFds) {
		struct pollfd *pollFds = multPlexFdsToPollFds(monitorFds);
		nfds_t nPollFds = getNumberOfPollFds(monitorFds);
		int nReady = poll(pollFds, nPollFds, Constants::multiplexTimeout);

		if (nReady < 0) {
			if (errno == EINTR) // TODO: @all: why is EINTR okay? What about the other codes? What about EAGAIN?
				return MultPlexFds(POLL);
			throw runtime_error(string("poll failed: ") + strerror(errno)); // TODO: @timo: make Errors::...
		}

		return getReadyPollFds(monitorFds, nReady, pollFds, nPollFds);
}

HttpServer::MultPlexFds HttpServer::getReadyFds(MultPlexFds& monitorFds) {
	switch (monitorFds.multPlexType) {
		case SELECT:
			throw std::logic_error("Getting ready fds from select not implemented yet"); break;
		case POLL:
			return doPoll(monitorFds); break;
		case EPOLL:
			throw std::logic_error("Getting ready fds from epoll not implemented yet"); break;
		default:
			throw std::logic_error("Getting ready fds from unknown method not implemented");
	}
}

int HttpServer::multPlexFdToRawFd(const MultPlexFds& readyFds, size_t i) {
	switch (readyFds.multPlexType) {
		case SELECT:
			throw std::logic_error("Converting seleet fd type to raw fd not implemented");
		case POLL:
			return readyFds.pollFds[i].fd;
		case EPOLL:
			throw std::logic_error("Converting epoll fd type to raw fd not implemented");
		default:
			throw std::logic_error("Converting unknown fd type to raw fd not implemented");
	}
}

void HttpServer::run() {
	while (true) {
		MultPlexFds readyFds = getReadyFds(_monitorFds); // block until: 1) new client
														 //              2) client that can be written to (response)
														 //              3) client that can be read from (request)
														 //              4) Constants::multiplexTimeout milliseconds is over
		handleReadyFds(readyFds); // 1) accept new client and add to pool
								  // 2) write pending data for Constants::chunkSize bytes (and remove this disposition if done writing)
								  // 3) read data for Constants::chunkSize bytes (or remove client if appropriate)
								  // 4) do nothing
	}
	/* NOTREACHED */
}
