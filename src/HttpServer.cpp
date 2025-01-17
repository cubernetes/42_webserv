#include <iostream>
#include <ostream>
#include <sys/socket.h> // TODO: @sonia: unnecessary header?
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

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
	_id(_idCntr++) {
	TRACE_DEFAULT_CTOR;
}

HttpServer::HttpServer(const HttpServer& other) :
	_serverFd(-1),
	_pollFds(other._pollFds),
	_running(other._running),
	_config(other._config),
	_id(_idCntr++) {
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
	int j = 8;

	while (j--) {
		int ready = poll(_pollFds.data(), _pollFds.size(), -1);
		if (ready < 0) {
			if (errno == EINTR)
				continue;
			Logger::logerror("Poll failed");
			break;
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
	
	if (bytesRead < 0) {
		if (errno != EWOULDBLOCK && errno != EAGAIN)
			closeConnection(clientFd);
		return;
	}
	
	if (bytesRead == 0) {
		closeConnection(clientFd);
		return;
	}
	
	// Simple response for now
	string response = "HTTP/1.1 200 OK\r\n"
					"Content-Length: 13\r\n"
					"Connection: close\r\n"
					"\r\n"
					"Hello World!\n";

	send(clientFd, response.c_str(), response.length(), 0);
	closeConnection(clientFd);
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
