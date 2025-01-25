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

using std::map;
using std::cout;
using std::vector;
using std::string;
using Constants::SELECT;
using Constants::POLL;
using Constants::EPOLL;
using Utils::SSTR;

HttpServer::~HttpServer() {
	TRACE_DTOR;
	// for (int i = 0; i < _listeningSockets.size(); ++i) {
	// 	if (_listeningSockets[i] >= 0)
	// 		close(_listeningSockets[i]);
	// } // TODO: @timo: remove, already handled below
	
	// for (int i = 0, fd; i < static_cast<int>(_pollFds.size()); ++i)
	// 	if ((fd = _pollFds[i].fd) >= 0)
	// 		close(fd); // TODO: @timo: remove, also already handled below

	closeAndRemoveAllMultPlexFd(_monitorFds);
}

HttpServer::HttpServer(const string& configPath) : _listeningSockets(), _monitorFds(Constants::defaultMultPlexType), _pollFds(_monitorFds.pollFds), _httpVersionString(Constants::httpVersionString), _rawConfig(readConfig(configPath)), _config(parseConfig(_rawConfig)), _mimeTypes(), _statusTexts(), _pendingWrites(), _pendingCloses(), _servers(), _defaultServers() {
	TRACE_ARG_CTOR(const string&, configPath);
	setup();
	initMimeTypes();
}

HttpServer::operator string() const {
	return ::repr(*this);
}

std::ostream& operator<<(std::ostream& os, const HttpServer& server) {
	return os << static_cast<string>(server);
}

bool HttpServer::setup() {
	for (ServerCtxs::const_iterator server = _config.second.begin();
			server != _config.second.end(); ++server) {
		ServerConfig serverConfig;
		const Arguments hostPort = getFirstDirective(server->first, "listen");
		struct addrinfo hints, *res;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		if (getaddrinfo(hostPort[0].c_str(), NULL, &hints, &res) != 0) {
			Logger::logError("Invalid IP address");
			return false;
		}
		serverConfig.ip = ((struct sockaddr_in*)res->ai_addr)->sin_addr;
		freeaddrinfo(res);
		
		serverConfig.port = std::atoi(hostPort[1].c_str());

		const Arguments names = getFirstDirective(server->first, "server_name");
		serverConfig.serverNames.insert(serverConfig.serverNames.end(), 
										names.begin(), names.end());
		
		serverConfig.directives = server->first;
		serverConfig.locations = server->second;
		
		if (!setupSocket(hostPort[0].c_str(), serverConfig.port))
			return false;
			
		_servers.push_back(serverConfig);

		AddrPort addr(serverConfig.ip, serverConfig.port);
		if (_defaultServers.find(addr) == _defaultServers.end())
			_defaultServers[addr] = _servers.back();
	}
	return true;
}

bool HttpServer::setupSocket(const string& ip, int port) {
	(void)ip;
	int listeningSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	_listeningSockets.push_back(listeningSocket);
	if (listeningSocket < 0) {
		Logger::logError("Failed to create socket");
		return false;
	}
	
	int opt = 1;
	if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		Logger::logError("Failed to set socket options");
		close(listeningSocket);
		return false;
	}
	
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	if (port > 0 && port <= SHRT_MAX) {
		address.sin_port = htons(static_cast<uint16_t>(port));
	} else {
		Logger::logError("Invalid port number");
		return false;
	}
	address.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(listeningSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
		Logger::logError("Failed to bind socket");
		close(listeningSocket);
		return false;
	}
	
	if (listen(listeningSocket, SOMAXCONN) < 0) {
		Logger::logError("Failed to listen on socket");
		close(listeningSocket);
		return false;
	}
	
	struct pollfd pfd;
	pfd.fd = listeningSocket;
	pfd.events = POLLIN;
	_monitorFds.pollFds.push_back(pfd);
	
	cout << "Server is listening on port " << port << std::endl;
	return true;
}

bool HttpServer::findMatchingServer(ServerConfig& serverConfig, const string& host, const struct in_addr& addr, int port) const {
	string serverName = host;
	size_t colonPos = serverName.find(':');
	if (colonPos != string::npos) 
		serverName = serverName.substr(0, colonPos);
	for (vector<ServerConfig>::const_iterator it = _servers.begin();
			it != _servers.end(); ++it) {
		if (it->port == port && memcmp(&it->ip, &addr, sizeof(struct in_addr)) == 0) {
			for (vector<string>::const_iterator name = it->serverNames.begin();
					name != it->serverNames.end(); ++name) {
				if (*name == serverName) {
					serverConfig = *it;
					return true;
				}
			}
		}
	}

	AddrPort addrPair(addr, port);
	DefaultServers::const_iterator it = _defaultServers.find(addrPair);
	if (it != _defaultServers.end()) {
		serverConfig = it->second;
		return true;
	}

	return false;
}

void HttpServer::queueWrite(int clientSocket, const string& data) {

	if (_pendingWrites.find(clientSocket) == _pendingWrites.end())
		_pendingWrites[clientSocket] = PendingWrite(data);
	else
		_pendingWrites[clientSocket].data += data;
	
	for (size_t i = 0; i < _pollFds.size(); ++i) {
		if (_pollFds[i].fd == clientSocket) {
			_pollFds[i].events |= POLLOUT;
			break ;
		}
	}
}

void HttpServer::writeToClient(int clientSocket) {

	map<int, PendingWrite>::iterator it = _pendingWrites.find(clientSocket);
	if (it == _pendingWrites.end()) {
	// No pending writes, remove POLLOUT from events
		for (size_t i = 0; i < _pollFds.size(); ++i) {
			if (_pollFds[i].fd == clientSocket) {
				_pollFds[i].events &= ~POLLOUT;
				break;
			}
		}

		if (_pendingCloses.find(clientSocket) != _pendingCloses.end()) {
			_pendingCloses.erase(clientSocket);
			removeClient(clientSocket);
		}
		return;
	}

	PendingWrite& pw = it->second;
	const char* data = pw.data.c_str() + pw.bytesSent;
	size_t remaining = std::min(Constants::chunkSize, pw.data.length() - pw.bytesSent); // TODO: @discuss: we don't want to send 10GB responses, that'll block the server for everyone; TODO: @discuss: is `chunkSize` the correct term?
	
	ssize_t bytesSent = send(clientSocket, data, remaining, 0);
	if (bytesSent < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			removeClient(clientSocket);
		}
		return;
	}
	
	pw.bytesSent += static_cast<size_t>(bytesSent);
	
	// Check if we've sent everything
	if (pw.bytesSent >= pw.data.length()) {
		_pendingWrites.erase(it);
		// Rm POLLOUT from events since we're done writing
		for (size_t i = 0; i < _pollFds.size(); ++i) {
			if (_pollFds[i].fd == clientSocket) {
				_pollFds[i].events &= ~POLLOUT;
				break;
			}
		}
	}
}

void HttpServer::addNewClient(int listeningSocket) {
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	
	int clientSocket = accept(listeningSocket, (struct sockaddr*)&clientAddr, &clientLen);
	if (clientSocket < 0) {
		if (errno != EWOULDBLOCK && errno != EAGAIN)
			Logger::logError("Accept failed");
		return;
	}
	
	struct pollfd pfd;
	pfd.fd = clientSocket;
	pfd.events = POLLIN | POLLOUT;
	pfd.revents = 0;
	_pollFds.push_back(pfd);
	
	cout << "New client connected. FD: " << clientSocket << std::endl;
}

bool HttpServer::findMatchingLocation(LocationCtx& location, const ServerConfig& serverConfig, const string& path) const {
	int bestIdx = -1;
	int bestScore = -1;
	for (int i = 0; i < static_cast<int>(serverConfig.locations.size()); ++i) {
		LocationCtx loc = serverConfig.locations[static_cast<size_t>(i)];
		const string& locPath = loc.first;
		pair<string::const_iterator, string::const_iterator> matcher = std::mismatch(locPath.begin(), locPath.end(), path.begin());
		int currentScore = static_cast<int>(std::distance(locPath.begin(), matcher.first));
		if (currentScore > bestScore) {
			bestScore = currentScore;
			bestIdx = i;
		}
	}
	if (bestIdx >= 0) {
		location = serverConfig.locations[static_cast<size_t>(bestIdx)];
		return true;
	}

	// TODO: @all: not good
	location = serverConfig.locations[0];
	return true; // TODO: maybe false, idk
}

void HttpServer::readFromClient(int clientSocket) {
	char buffer[CONSTANTS_CHUNK_SIZE];
	ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
	
	if (bytesRead <= 0) {
		if (bytesRead == 0 || (bytesRead < 0 && errno != EWOULDBLOCK && errno != EAGAIN)) // TODO: @all: quote subject: "checking the value of errno is strictly forbidden after a read or write operation"
																						  // recv/recvfrom and send/sendto are also read/write ops (just specialized for sockets).
																						  // setting the fd to NONBLOCK and then checking whether errno is EWOULDBLOCK or EAGAIN is called the "nonblocking IO" model, which is forbidden
																						  // namely, we have to implementing "I/O multiplexing". this video is really great: youtu.be/I5j9TBcqe_Q
			removeClient(clientSocket);
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
	
	int port = ntohs(addr.sin_port);
	// Get host from request headers
	string host;
	if (request.headers.find("Host") != request.headers.end()) {
		host = request.headers.at("Host");
		size_t colonPos = host.find(':');
		if (colonPos != string::npos)
			host = host.substr(0, colonPos);
	}

	ServerConfig server;
	if (!findMatchingServer(server, host, addr.sin_addr, port)) {
		sendError(clientSocket, 404);
		return;
	}
	LocationCtx location;
	if (!findMatchingLocation(location, server, request.path)) {
		sendError(clientSocket, 404);
		return;
	}
	
	if (request.method == "GET")
		serveStaticContent(clientSocket, request, location);
	else {
		string response = _httpVersionString + " " + SSTR(501) + statusTextFromCode(501) + "\r\n"
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

void HttpServer::initMimeTypes() {

	// Web content
	_mimeTypes["html"] = "text/html";
	_mimeTypes["htm"] = "text/html";
	_mimeTypes["css"] = "text/css";
	_mimeTypes["js"] = "application/javascript";
	_mimeTypes["xml"] = "application/xml";
	_mimeTypes["json"] = "application/json";

	// Text files
	_mimeTypes["txt"] = "text/plain";
	_mimeTypes["csv"] = "text/csv";
	_mimeTypes["md"] = "text/markdown";
	_mimeTypes["sh"] = "text/x-shellscript";

	// Images
	_mimeTypes["jpg"] = "image/jpeg";
	_mimeTypes["jpeg"] = "image/jpeg";
	_mimeTypes["png"] = "image/png";
	_mimeTypes["gif"] = "image/gif";
	_mimeTypes["svg"] = "image/svg+xml";
	_mimeTypes["ico"] = "image/x-icon";
	_mimeTypes["webp"] = "image/webp";

	// Documents
	_mimeTypes["pdf"] = "application/pdf";
	_mimeTypes["doc"] = "application/msword";
	_mimeTypes["docx"] = "application/msword";
	_mimeTypes["xls"] = "application/vnd.ms-excel";
	_mimeTypes["xlsx"] = "application/vnd.ms-excel";
	_mimeTypes["zip"] = "application/zip";

	// Multimedia
	_mimeTypes["mp3"] = "audio/mpeg";
	_mimeTypes["mp4"] = "video/mp4";
	_mimeTypes["webm"] = "video/webm";

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

void HttpServer::initStatusTexts() {
	_statusTexts[100] = "Continue";
	_statusTexts[101] = "Switching Protocols";
	_statusTexts[102] = "Processing";
	_statusTexts[103] = "Early Hints";
	_statusTexts[200] = "OK";
	_statusTexts[201] = "Created";
	_statusTexts[202] = "Accepted";
	_statusTexts[203] = "Non-Authoritative Information";
	_statusTexts[204] = "No Content";
	_statusTexts[205] = "Reset Content";
	_statusTexts[206] = "Partial Content";
	_statusTexts[207] = "Multi-Status";
	_statusTexts[208] = "Already Reported";
	_statusTexts[226] = "IM Used";
	_statusTexts[300] = "Multiple Choices";
	_statusTexts[301] = "Moved Permanently";
	_statusTexts[302] = "Found";
	_statusTexts[303] = "See Other";
	_statusTexts[304] = "Not Modified";
	_statusTexts[305] = "Use Proxy";
	_statusTexts[306] = "Switch Proxy";
	_statusTexts[307] = "Temporary Redirect";
	_statusTexts[308] = "Permanent Redirect";
	_statusTexts[404] = "error on Wikimedia";
	_statusTexts[400] = "Bad Request";
	_statusTexts[401] = "Unauthorized";
	_statusTexts[402] = "Payment Required";
	_statusTexts[403] = "Forbidden";
	_statusTexts[404] = "Not Found";
	_statusTexts[405] = "Method Not Allowed";
	_statusTexts[406] = "Not Acceptable";
	_statusTexts[407] = "Proxy Authentication Required";
	_statusTexts[408] = "Request Timeout";
	_statusTexts[409] = "Conflict";
	_statusTexts[410] = "Gone";
	_statusTexts[411] = "Length Required";
	_statusTexts[412] = "Precondition Failed";
	_statusTexts[413] = "Payload Too Large";
	_statusTexts[414] = "URI Too Long";
	_statusTexts[415] = "Unsupported Media Type";
	_statusTexts[416] = "Range Not Satisfiable";
	_statusTexts[417] = "Expectation Failed";
	_statusTexts[418] = "I'm a teapot";
	_statusTexts[421] = "Misdirected Request";
	_statusTexts[422] = "Unprocessable Content";
	_statusTexts[423] = "Locked";
	_statusTexts[424] = "Failed Dependency";
	_statusTexts[425] = "Too Early";
	_statusTexts[426] = "Upgrade Required";
	_statusTexts[428] = "Precondition Required";
	_statusTexts[429] = "Too Many Requests";
	_statusTexts[431] = "Request Header Fields Too Large";
	_statusTexts[451] = "Unavailable For Legal Reasons";
	_statusTexts[500] = "Internal Server Error";
	_statusTexts[501] = "Not Implemented";
	_statusTexts[502] = "Bad Gateway";
	_statusTexts[503] = "Service Unavailable";
	_statusTexts[504] = "Gateway Timeout";
	_statusTexts[505] = "HTTP Version Not Supported";
	_statusTexts[506] = "Variant Also Negotiates";
	_statusTexts[507] = "Insufficient Storage";
	_statusTexts[508] = "Loop Detected";
	_statusTexts[510] = "Not Extended";
	_statusTexts[511] = "Network Authentication Required";
}

string HttpServer::wrapInHtmlBody(const string& text) {
	return "<html><body>" + text + "</body></html>";
}

void HttpServer::sendError(int clientSocket, int statusCode) {
	sendString(clientSocket, wrapInHtmlBody(
		"<h1>" + SSTR(statusCode) + " " + statusTextFromCode(statusCode) + "</h1>"
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
	map<string, string>::const_iterator it = _mimeTypes.find(ext);
	if (it != _mimeTypes.end())
		return it->second; // MIME type found
	return Constants::defaultMimeType; // MIME type not found
}

bool HttpServer::isListeningSocket(int fd) {
	return std::find(_listeningSockets.begin(), _listeningSockets.end(), fd) != _listeningSockets.end();
}

void HttpServer::handleReadyFds(const MultPlexFds& readyFds) {
	int nReadyFds = static_cast<int>(readyFds.fdStates.size());
	for (int i = 0; i < nReadyFds; ++i) {
		int fd = multPlexFdToRawFd(readyFds, i);
		if (readyFds.fdStates[static_cast<size_t>(i)] == FD_READABLE) {
			if (isListeningSocket(fd))
				addNewClient(fd);
			else
				readFromClient(fd);
		} else if (readyFds.fdStates[static_cast<size_t>(i)] == FD_WRITEABLE)
			writeToClient(fd);
		else
			throw std::logic_error("Cannot handle fd other than readable or writable at this step"); // TODO: @timo: proper logging
	}
}

struct pollfd *HttpServer::getPollFds(const MultPlexFds& fds) {
	return (struct pollfd*)&fds.pollFds[0];
}

nfds_t HttpServer::getNumberOfPollFds(const MultPlexFds& fds) {
	return static_cast<nfds_t>(fds.pollFds.size());
}

void HttpServer::removeSelectFd(MultPlexFds& monitorFds, int fd) {
	FD_CLR(fd, &monitorFds.selectFdSet);
}

void HttpServer::closeAndRemoveAllSelectFd(MultPlexFds& monitorFds) {
	for (int i = 0; i < FD_SETSIZE; ++i) {
		if (FD_ISSET(i, &monitorFds.selectFdSet)) {
			FD_CLR(i, &monitorFds.selectFdSet);
			close(i);
		}
	}
	FD_ZERO(&monitorFds.selectFdSet);
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
			removeSelectFd(monitorFds, fd); break;
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
			closeAndRemoveAllSelectFd(monitorFds); break;
		case POLL:
			closeAndRemoveAllPollFd(monitorFds); break;
		case EPOLL:
			throw std::logic_error("Removing epoll type fds not implemented yet"); break;
		default:
			throw std::logic_error("Removing unknown type of fd not implemented");
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
		struct pollfd *pollFds = getPollFds(monitorFds);
		nfds_t nPollFds = getNumberOfPollFds(monitorFds);
		int nReady = poll(pollFds, nPollFds, Constants::multiplexTimeout);

		if (nReady < 0) {
			if (errno == EINTR) // TODO: @all: why is EINTR okay? What about the other codes? What about EAGAIN?
				return MultPlexFds(POLL);
			throw std::runtime_error(string("Poll failed: ") + strerror(errno)); // TODO: @timo: make Errors::...
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

int HttpServer::multPlexFdToRawFd(const MultPlexFds& readyFds, int i) {
	switch (readyFds.multPlexType) {
		case SELECT:
			return readyFds.selectFds[static_cast<size_t>(i)];
		case POLL:
			return readyFds.pollFds[static_cast<size_t>(i)].fd;
		case EPOLL:
			return readyFds.epollFds[static_cast<size_t>(i)].data.fd;
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
