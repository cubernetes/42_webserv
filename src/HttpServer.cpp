#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <limits.h>

#include "HttpServer.hpp"
#include "conf.hpp"
#include "Logger.hpp"

using std::cout;
using std::string;
using std::stringstream;

// Initialize static counter
unsigned int HttpServer::_idCntr = 0;

HttpServer::~HttpServer() {
	TRACE_DTOR;
		
	if (server_fd >= 0)
		close(server_fd);
	
	for (std::vector<struct pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {
		if (it->fd >= 0 && it->fd != server_fd)
			close(it->fd);
	}
}

HttpServer::HttpServer() : 
	server_fd(-1),
	poll_fds(),
	running(false),
	config(),
	_id(_idCntr++) {
	reflect(); TRACE_DEFAULT_CTOR;
}

HttpServer::HttpServer(const HttpServer& other) :
	server_fd(-1),
	poll_fds(other.poll_fds),
	running(other.running),
	config(other.config),
	_id(_idCntr++) {
	reflect(); TRACE_COPY_CTOR;
}

HttpServer& HttpServer::operator=(HttpServer other) {
	TRACE_COPY_ASSIGN_OP;
	swap(other);
	return *this;
}

void HttpServer::swap(HttpServer& other) {
	TRACE_SWAP_BEGIN;
	std::swap(server_fd, other.server_fd);
	std::swap(poll_fds, other.poll_fds);
	std::swap(running, other.running);
	std::swap(config, other.config);
	std::swap(_id, other._id);
	TRACE_SWAP_END;
}

HttpServer::operator string() const { return ::repr(*this); }

bool HttpServer::setup(const t_config& conf) {
	//config = conf;
	(void)conf;
	return setupSocket("127.0.0.1", 8080);  // Hardcoded for now
}

bool HttpServer::setupSocket(const std::string& ip, int port) {
	(void)ip;
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		Logger::logerror("Failed to create socket");
		return false;
	}
	
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		Logger::logerror("Failed to set socket options");
		close(server_fd);
		return false;
	}
	
	if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0) {
		Logger::logerror("Failed to set non-blocking socket");
		close(server_fd);
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
	
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		Logger::logerror("Failed to bind socket");
		close(server_fd);
		return false;
	}
	
	if (listen(server_fd, SOMAXCONN) < 0) {
		Logger::logerror("Failed to listen on socket");
		close(server_fd);
		return false;
	}
	
	struct pollfd pfd;
	pfd.fd = server_fd;
	pfd.events = POLLIN;
	poll_fds.push_back(pfd);
	
	cout << "Server is listening on port " << port << std::endl;
	return true;
}

void HttpServer::run() {
	running = true;
	int j = 8;

	while (j--) {
		int ready = poll(poll_fds.data(), poll_fds.size(), -1);
		if (ready < 0) {
			if (errno == EINTR)
				continue;
			Logger::logerror("Poll failed");
			break;
		}
		
		for (size_t i = 0; i < poll_fds.size(); i++) {
			if (poll_fds[i].revents == 0)
				continue;
				
			if (poll_fds[i].revents & POLLIN) {
				if (poll_fds[i].fd == server_fd)
					handleNewConnection();
				else
					handleClientData(poll_fds[i].fd);
			}
			
			if (poll_fds[i].revents & (POLLHUP | POLLERR))
				closeConnection(poll_fds[i].fd);
		}
	}
}

void HttpServer::handleNewConnection() {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd < 0) {
		if (errno != EWOULDBLOCK && errno != EAGAIN)
			Logger::logerror("Accept failed");
		return;
	}
	
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
		Logger::logerror("Failed to set client socket non-blocking");
		close(client_fd);
		return;
	}
	
	struct pollfd pfd;
	pfd.fd = client_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	poll_fds.push_back(pfd);
	
	cout << "New client connected. FD: " << client_fd << std::endl;
}

void HttpServer::handleClientData(int client_fd) {
	char buffer[4096];
	ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
	
	if (bytes_read < 0) {
		if (errno != EWOULDBLOCK && errno != EAGAIN)
			closeConnection(client_fd);
		return;
	}
	
	if (bytes_read == 0) {
		closeConnection(client_fd);
		return;
	}
	
	// Simple response for now
	string response = "HTTP/1.1 200 OK\r\n"
					"Content-Length: 13\r\n"
					"Connection: close\r\n"
					"\r\n"
					"Hello World!\n";

	send(client_fd, response.c_str(), response.length(), 0);
	closeConnection(client_fd);
}

void HttpServer::closeConnection(int client_fd) {
	close(client_fd);
	removePollFd(client_fd);
}

void HttpServer::removePollFd(int fd) {
	for (std::vector<struct pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {
		if (it->fd == fd) {
			poll_fds.erase(it);
			break;
		}
	}
}

void swap(HttpServer& a, HttpServer& b) {
	a.swap(b);
}

std::ostream& operator<<(std::ostream& os, const HttpServer& server) {
	return os << static_cast<string>(server);
}
