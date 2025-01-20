#include <iostream>
#include <ostream>
#include <netinet/in.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Config.hpp"
#include "Repr.hpp"

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
	_mimeTypes() {
	TRACE_DEFAULT_CTOR;
	initMimeTypes();
}

HttpServer::HttpServer(const HttpServer& other) :
	_serverFd(-1),
	_pollFds(other._pollFds),
	_running(other._running),
	_config(other._config),
	_id(_idCntr++),
	_mimeTypes() {
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
	return setupSocket("127.0.0.1", 8080);  // Hardcoded for now
}

bool HttpServer::setupSocket(const std::string& ip, int port) {
	(void)ip;
	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
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
	
	if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0) {
		Logger::logerror("Failed to set non-blocking socket");
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

void HttpServer::run() {
	_running = true;

	while (_running){

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
	
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
		Logger::logerror("Failed to set client socket non-blocking");
		close(clientFd);
		return;
	}
	
	struct pollfd pfd;
	pfd.fd = clientFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollFds.push_back(pfd);
	
	cout << "New client connected. FD: " << clientFd << std::endl;
	cout << *this << '\n';
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

HttpServer::HttpRequest HttpServer::parseHttpRequest(const char *buffer)
{

	HttpRequest request;
	std::istringstream requestStream(buffer);
	string line;

	std::getline(requestStream, line);

	std::istringstream requestLine(line);
	requestLine	>> request.method >> request.path >> request.httpVersion;

	while (std::getline(requestStream, line) && line != "\r") {
		size_t colonPos = line.find(':');
		if (colonPos != string::npos)
		{
			string key = line.substr(0, colonPos);
			string value = line.substr(colonPos + 1);
			value.erase(0, value.find_first_not_of(" "));
			value.erase(value.find_last_not_of("\r") + 1);
			request.headers[key] = value;
		}
	}
	return request;
}

bool HttpServer::validateServerConfig(int clientFd, const ServerCtx& serverConfig, string& rootDir, string& defaultIndex)
{
	if (serverConfig.first.find("root") == serverConfig.first.end() ||
			serverConfig.first.at("root").empty()) {
			sendError(clientFd, 500, "Internal Server Error");
			return false;
		}
		rootDir = serverConfig.first.at("root")[0];

		if (serverConfig.first.find("index") == serverConfig.first.end() ||
			serverConfig.first.at("index").empty()) {
			sendError(clientFd, 500, "Internal Server Error");
			return false;
		}
		defaultIndex = serverConfig.first.at("index")[0];
		return true;
}

bool HttpServer::validatePath(int clientFd, const string& path) {
		if (path.find("..") != string::npos || 
			path.find("//") != string::npos || 
			path[0] != '/' || 
			path.find(':') != string::npos) {
			sendError(clientFd, 403, "Forbidden");
			return false;
		}
		return true;
}

bool HttpServer::handleDirectoryRedirect(int clientFd, const HttpRequest& request, string& filePath, 
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
			sendError(clientFd, 404, "Not Found");
			return false;
		}
		return true;
}

void HttpServer::sendFileContent(int clientFd, const string& filePath)
{
		std::ifstream file(filePath.c_str(), std::ios::binary);
		if (!file) {
			sendError(clientFd, 403, "Forbidden");
			return;
		}

		file.seekg(0, std::ios::end);
		size_t fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		std::ostringstream headers;
		headers << "HTTP/1.1 200 OK\r\n"
				<< "Content-Length: " << fileSize << "\r\n"
				<< "Content-Type: " << getMimeType(filePath) << "\r\n"
				<< "Connection: close\r\n\r\n";

		string headerStr = headers.str();
		send(clientFd, headerStr.c_str(), headerStr.length(), 0);

		char buffer[4096];
		while(file.read(buffer, sizeof(buffer)))
			send(clientFd, buffer, file.gcount(), 0);
		if (file.gcount() > 0)
			send(clientFd, buffer, file.gcount(), 0);
		closeConnection(clientFd);
}

void HttpServer::handleGetRequest(int clientFd, const HttpRequest& request)
{

	//TODO: @sonia: multiple servers
	if (_config.second.empty()) {
		sendError(clientFd, 500, "Internal Server Error");
		return;
	}

	const ServerCtx& serverConfig = _config.second[0];
	string rootDir, defaultIndex;
	if (!validateServerConfig(clientFd, serverConfig, rootDir, defaultIndex))
		return;
	if (!validatePath(clientFd, request.path))
		return;

	string filePath = rootDir + request.path;
	if (filePath[filePath.length() - 1] == '/')
		filePath += defaultIndex;

	struct stat fileStat;
	if (stat(filePath.c_str(), &fileStat) == -1) {
		sendError(clientFd, 404, "Not Found");
		return;
	}

	if (S_ISDIR(fileStat.st_mode)) {
		if (!handleDirectoryRedirect(clientFd, request, filePath, defaultIndex, fileStat))
			return;
	}

	sendFileContent(clientFd, filePath);
}

void HttpServer::sendError(int clientFd, int statusCode, const string& statusText)
{
	std::ostringstream response;
	response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n"
			<< "Content-Type: text/html\r\n"
			<< "Connection: close\r\n\r\n"
			<< "\r\n"
			<< "<html><body><h1>" << statusCode << " " << statusText << "</h1></body></html>";
	string responseStr = response.str();
	send(clientFd, responseStr.c_str(), responseStr.length(), 0);
	closeConnection(clientFd);
}

string HttpServer::getMimeType(const string& path)
{
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
