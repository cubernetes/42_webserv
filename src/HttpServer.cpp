#include <iostream>
#include <ostream>
#include <cstdlib>
#include <fstream>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Config.hpp"
#include "Repr.hpp"
#include "DirectoryIndexing.hpp"

using std::swap;
using std::cout;
using std::string;

HttpServer::~HttpServer() {
	TRACE_DTOR;
	if (_serverFd >= 0)
		close(_serverFd);
	
	for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it) {
		if (it->fd >= 0 && it->fd != _serverFd)
			close(it->fd);
	}
}

HttpServer::HttpServer() : 
	_serverFd(-1),
	_pollFds(),
	_running(false),
	_config(),
	_id(_idCntr++),
	_mimeTypes(),
	_pendingWrites(),
	_pendingClose() {
	TRACE_DEFAULT_CTOR;
	initMimeTypes();
}

HttpServer::HttpServer(const HttpServer& other) :
	_serverFd(-1),
	_pollFds(other._pollFds),
	_running(other._running),
	_config(other._config),
	_id(_idCntr++),
	_mimeTypes(),
	_pendingWrites(),
	_pendingClose() {
	TRACE_COPY_CTOR;
}

// Instance tracking
unsigned int HttpServer::_idCntr = 0;

// copy swap idiom
HttpServer& HttpServer::operator=(HttpServer other) {
	TRACE_COPY_ASSIGN_OP;
	swap(other);
	return *this;
}

// Getters
int HttpServer::get_serverFd() const { return _serverFd; }
const std::vector<struct pollfd>& HttpServer::get_pollFds() const { return _pollFds; }
bool HttpServer::get_running() const { return _running; }
const Config& HttpServer::get_config() const { return _config; }
unsigned int HttpServer::get_id() const { return _id; }

void HttpServer::swap(HttpServer& other) {
	TRACE_SWAP_BEGIN;
	::swap(_serverFd, other._serverFd);
	::swap(_pollFds, other._pollFds);
	::swap(_running, other._running);
	::swap(_config, other._config);
	TRACE_SWAP_END;
}

HttpServer::operator string() const {
	return ::repr(*this);
}

void swap(HttpServer& a, HttpServer& b) {
	a.swap(b);
}

std::ostream& operator<<(std::ostream& os, const HttpServer& server) {
	return os << static_cast<string>(server);
}
// end of boilerplate


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
}

bool HttpServer::setup(const Config& conf) {
	_config = conf;
	const Arguments hostPort = getFirstDirective(conf.second[0].first, "listen");
	const char *host = hostPort[0].c_str();
	int port = std::atoi(hostPort[1].c_str());
	return setupSocket(host, port);
}

bool HttpServer::setupSocket(const std::string& ip, int port) {
	(void)ip;
	_serverFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (_serverFd < 0) {
		Logger::logerror("Failed to create socket");
		return false;
	}
	
	int opt = 1;
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		Logger::logerror("Failed to set socket options");
		close(_serverFd);
		return false;
	}
	
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	if (port > 0 && port <= SHRT_MAX) {
		address.sin_port = htons(static_cast<uint16_t>(port));
	} else {
		Logger::logerror("Invalid port number");
		return false;
	}
	address.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(_serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		Logger::logerror("Failed to bind socket");
		close(_serverFd);
		return false;
	}
	
	if (listen(_serverFd, SOMAXCONN) < 0) {
		Logger::logerror("Failed to listen on socket");
		close(_serverFd);
		return false;
	}
	
	struct pollfd pfd;
	pfd.fd = _serverFd;
	pfd.events = POLLIN;
	_pollFds.push_back(pfd);
	
	cout << "Server is listening on port " << port << std::endl;
	return true;
}

void HttpServer::queueWrite(int clientFd, const string& data) {

	if (_pendingWrites.find(clientFd) == _pendingWrites.end())
		_pendingWrites[clientFd] = PendingWrite(data);
	else
		_pendingWrites[clientFd].data += data;
	
	for (size_t i = 0; i < _pollFds.size(); ++i) {
		if (_pollFds[i].fd == clientFd) {
			_pollFds[i].events |= POLLOUT;
			break ;
		}
	}
}

void HttpServer::handleClientWrite(int clientFd) {

	std::map<int, PendingWrite>::iterator it = _pendingWrites.find(clientFd);
	if (it == _pendingWrites.end()) {
	// No pending writes, remove POLLOUT from events
		for (size_t i = 0; i < _pollFds.size(); ++i) {
			if (_pollFds[i].fd == clientFd) {
				_pollFds[i].events &= ~POLLOUT;
				break;
			}
		}

		if (_pendingClose.find(clientFd) != _pendingClose.end()) {
			_pendingClose.erase(clientFd);
			closeConnection(clientFd);
		}
		return;
	}

	PendingWrite& pw = it->second;
	const char* data = pw.data.c_str() + pw.bytesSent;
	size_t remaining = pw.data.length() - pw.bytesSent;
	
	ssize_t bytesSent = send(clientFd, data, remaining, 0);
	if (bytesSent < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			closeConnection(clientFd);
		}
		return;
	}
	
	pw.bytesSent += static_cast<size_t>(bytesSent);
	
	// Check if we've sent everything
	if (pw.bytesSent >= pw.data.length()) {
		_pendingWrites.erase(it);
		// Rm POLLOUT from events since we're done writing
		for (size_t i = 0; i < _pollFds.size(); ++i) {
			if (_pollFds[i].fd == clientFd) {
				_pollFds[i].events &= ~POLLOUT;
				break;
			}
		}
	}
}

void HttpServer::run() {
	_running = true;

	while (_running) {

		int ready = poll(_pollFds.data(), _pollFds.size(), -1);
		if (ready < 0) {
			if (errno == EINTR)
				continue ;
			Logger::logerror("Poll failed");
			break ;
		}
		
		for (size_t i = 0; i < _pollFds.size(); i++) {
			if (_pollFds[i].revents == 0)
				continue;
				
			if (_pollFds[i].revents & POLLIN) {
				if (_pollFds[i].fd == _serverFd)
					handleNewConnection();
				else
					handleClientData(_pollFds[i].fd);
			}

			if (_pollFds[i].revents & POLLOUT)
				handleClientWrite(_pollFds[i].fd);
			
			if (_pollFds[i].revents & (POLLHUP | POLLERR))
				closeConnection(_pollFds[i].fd);
		}
	}
}

void HttpServer::handleNewConnection() {
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	
	int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &clientLen);
	if (clientFd < 0) {
		if (errno != EWOULDBLOCK && errno != EAGAIN)
			Logger::logerror("Accept failed");
		return;
	}
	
	struct pollfd pfd;
	pfd.fd = clientFd;
	pfd.events = POLLIN | POLLOUT;
	pfd.revents = 0;
	_pollFds.push_back(pfd);
	
	cout << "New client connected. FD: " << clientFd << std::endl;
}

void HttpServer::handleClientData(int clientFd) {
	char buffer[4096];
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
	
	if (bytesRead <= 0) {
		if (bytesRead == 0 || (bytesRead < 0 && errno != EWOULDBLOCK && errno != EAGAIN))
			closeConnection(clientFd);
		return;
	}
	
	buffer[bytesRead] = '\0';
	HttpRequest request = parseHttpRequest(buffer);
	
	if (request.method == "GET")
		handleGetRequest(clientFd, request);
	else {
		string response = "HTTP/1.1 501 Not Implemented\r\n"
					"Content-Length: 22\r\n"
					"Connection: close\r\n"
					"\r\n"
					"Method not implemented\n";

		send(clientFd, response.c_str(), response.length(), 0);
		closeConnection(clientFd);
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

bool HttpServer::validateServerConfig(int clientFd, const ServerCtx& serverConfig, string& rootDir, string& defaultIndex) {
	if (!directiveExists(serverConfig.first, "root") ||
			getFirstDirective(serverConfig.first, "root").empty()) {
			sendError(serverConfig, clientFd, 500, "Internal Server Error");
			return false;
		}
		rootDir = getFirstDirective(serverConfig.first, "root")[0];

		if (!directiveExists(serverConfig.first, "index") ||
			getFirstDirective(serverConfig.first, "index").empty()) {
			sendError(serverConfig, clientFd, 500, "Internal Server Error");
			return false;
		}
		defaultIndex = getFirstDirective(serverConfig.first, "index")[0];
		return true;
}

bool HttpServer::validatePath(const ServerCtx& serverConfig, int clientFd, const string& path) {
		if (path.find("..") != string::npos || 
			path.find("//") != string::npos || 
			path[0] != '/' || 
			path.find(':') != string::npos) {
			sendError(serverConfig, clientFd, 403, "Forbidden");
			return false;
		}
		return true;
}

bool HttpServer::handleDirectoryRedirect(const ServerCtx& serverConfig, int clientFd, const HttpRequest& request, string& filePath, 
							   const string& defaultIndex, struct stat& fileStat) {
		if (request.path[request.path.length() - 1] != '/') {
			string redirectUrl = request.path + "/";
			std::ostringstream response;
			response << "HTTP/1.1 301 Moved Permanently\r\n"
					<< "Location: " << redirectUrl << "\r\n"
					<< "Connection: close\r\n\r\n";
			string responseStr = response.str();
			send(clientFd, responseStr.c_str(), responseStr.length(), 0);
			closeConnection(clientFd);
			return false;
		}
		filePath += defaultIndex;
		if (stat(filePath.c_str(), &fileStat) == -1) {
			sendError(serverConfig, clientFd, 404, "Not Found");
			return false;
		}
		return true;
}

void HttpServer::sendFileContent(const ServerCtx& serverConfig, int clientFd, const string& filePath) {
		std::ifstream file(filePath.c_str(), std::ios::binary);
		if (!file) {
			sendError(serverConfig, clientFd, 403, "Forbidden");
			return;
		}

		file.seekg(0, std::ios::end);
		long fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		std::ostringstream headers;
		headers << "HTTP/1.1 200 OK\r\n"
				<< "Content-Length: " << fileSize << "\r\n"
				<< "Content-Type: " << getMimeType(filePath) << "\r\n"
				<< "Connection: close\r\n\r\n";

		queueWrite(clientFd, headers.str());

		char buffer[4096];
		while(file.read(buffer, sizeof(buffer)))
			queueWrite(clientFd, string(buffer, static_cast<size_t>(file.gcount())));
		if (file.gcount() > 0)
			queueWrite(clientFd, string(buffer, static_cast<size_t>(file.gcount())));
		_pendingClose.insert(clientFd);
}

void HttpServer::handleGetRequest(int clientFd, const HttpRequest& request) {
	const ServerCtx& serverConfig = _config.second[0];

	//TODO: @sonia: multiple servers
	if (_config.second.empty()) {
		sendError(serverConfig, clientFd, 500, "Internal Server Error");
		return;
	}

	string rootDir, defaultIndex;
	if (!validateServerConfig(clientFd, serverConfig, rootDir, defaultIndex))
		return;
	if (!validatePath(serverConfig, clientFd, request.path))
		return;

	struct stat fileStat;
	string filePath = rootDir + request.path;
	int fileExists = stat(filePath.c_str(), &fileStat) == 0;

	if (filePath[filePath.length() - 1] == '/') {
		struct stat indexFileStat;
		string indexPath = filePath + defaultIndex;

		if (stat(indexPath.c_str(), &indexFileStat) == -1) {
			if (getFirstDirective(serverConfig.first, "autoindex")[0] == "off") {
				sendError(serverConfig, clientFd, 403, "Forbidden");
				return;
			}
			if (fileExists)
				sendText(clientFd, indexDirectory(request.path, filePath));
			sendError(serverConfig, clientFd, 404, "Not Found");
			return;
		}
		filePath == indexPath;
		fileExists = stat(filePath.c_str(), &fileStat) == 0;
	}

	if (!fileExists) {
		sendError(serverConfig, clientFd, 404, "Not Found");
		return;
	}

	if (S_ISDIR(fileStat.st_mode)) {
		if (!handleDirectoryRedirect(serverConfig, clientFd, request, filePath, defaultIndex, fileStat))
			return;
	}

	if (S_ISREG(fileStat.st_mode))
		sendFileContent(serverConfig, clientFd, filePath);
	else
		sendError(serverConfig, clientFd, 404, "Not Found"); // sometimes it's also 403, or 500, but haven't figured out the pattern yet
}

string HttpServer::getErrorPagePath(int statusCode, const ServerCtx& serverConfig) {
	ArgResults allErrPageDirs = getAllDirectives(serverConfig.first, "error_page");
	for (ArgResults::const_iterator args = allErrPageDirs.begin(); args != allErrPageDirs.end(); ++args) {
		Arguments::const_iterator code = args->begin(), before = --args->end();
		for (; code != before; ++code) {
			if (atoi(code->c_str()) == statusCode) {
				return args->back();
			}
		}
	}
	return "";
}

void HttpServer::sendError(const ServerCtx& serverConfig, int clientFd, int statusCode, const string& statusText) {
	std::ostringstream	response;
	std::ostringstream	body;

	string errorPagePath = getErrorPagePath(statusCode, serverConfig);
	if (errorPagePath.empty())
		body << "<html><body><h1>" << statusCode << " " << statusText << "</h1></body></html>";
	else {
		std::ifstream errorPageFile((getFirstDirective(serverConfig.first, "root")[0] + errorPagePath).c_str());
		if (!errorPageFile)
			body << "<html><body><h1>" << statusCode << " " << statusText << "</h1></body></html>";
		else
			body << errorPageFile.rdbuf();
	}
	string bodyStr = body.str();
	
	response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n"
			<< "Content-Type: text/html\r\n"
			<< "Content-Length: " << bodyStr.length() << "\r\n"
			<< "Connection: close\r\n"
			<< "\r\n"
			<< bodyStr;
	queueWrite(clientFd, response.str());
	_pendingClose.insert(clientFd);
}

void HttpServer::sendText(int clientFd, const string& text) {
	std::ostringstream	response;
	std::ostringstream	body;

	body << "<html><body>" << text << "</body></html>";
	string bodyStr = body.str();
	
	response << "HTTP/1.1 200 OK\r\n"
			<< "Content-Type: text/html\r\n"
			<< "Content-Length: " << bodyStr.length() << "\r\n"
			<< "Connection: close\r\n"
			<< "\r\n"
			<< bodyStr;
	queueWrite(clientFd, response.str());
	_pendingClose.insert(clientFd);
}

string HttpServer::getMimeType(const string& path) {
	size_t dotPos = path.find_last_of('.');
	if (dotPos == string::npos)
		return "application/octet-stream";
	
	string ext = path.substr(dotPos + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	std::map<string, string>::const_iterator it = _mimeTypes.find(ext);
	if (it != _mimeTypes.end())
		return it->second;
	return "application/octet-stream";
}

void HttpServer::closeConnection(int clientFd) {
	close(clientFd);
	removePollFd(clientFd);
}

void HttpServer::removePollFd(int fd) {
	for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it) {
		if (it->fd == fd) {
			_pollFds.erase(it);
			break;
		}
	}
}
