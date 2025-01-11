#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include "HttpServer.hpp"
#include "repr.hpp"

using std::cout;
using std::string;
using std::stringstream;

// Initialize static counter
unsigned int HttpServer::_idCntr = 0;

HttpServer::~HttpServer() {
	if (Logger::trace())
		cout << ANSI_PUNCT "~" << *this << '\n';
		
	if (server_fd >= 0)
		close(server_fd);
	
	for (std::vector<struct pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {
		if (it->fd >= 0 && it->fd != server_fd)
			close(it->fd);
	}
}

HttpServer::HttpServer() : 
	server_fd(-1), 
	running(false),
	_id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "HttpServer" ANSI_PUNCT "() -> " << *this << '\n';
}

HttpServer::HttpServer(const HttpServer& other) :
	server_fd(-1),
	running(other.running),
	config(other.config),
	_id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "HttpServer" ANSI_PUNCT "(" << ::repr(other) << ANSI_PUNCT ") -> " << *this << '\n';
}

HttpServer& HttpServer::operator=(HttpServer other) {
	if (Logger::trace())
		cout << ANSI_KWRD "HttpServer" ANSI_PUNCT "& " ANSI_KWRD "HttpServer" ANSI_PUNCT "::" 
			ANSI_FUNC "operator" ANSI_PUNCT "=(" << ::repr(other) << ANSI_PUNCT ")" ANSI_RST "\n";
	swap(other);
	return *this;
}

void HttpServer::swap(HttpServer& other) {
	if (Logger::trace()) {
		cout << ANSI_CMT "<Swapping HttpServer *this:" ANSI_RST "\n";
		cout << *this << '\n';
		cout << ANSI_CMT "with the following HttpServer object:" ANSI_RST "\n";
		cout << other << '\n';
	}
	
	std::swap(server_fd, other.server_fd);
	std::swap(poll_fds, other.poll_fds);
	std::swap(running, other.running);
	std::swap(config, other.config);
	std::swap(_id, other._id);
	
	if (Logger::trace())
		cout << ANSI_CMT "HttpServer swap done>" ANSI_RST "\n";
}

string HttpServer::repr() const {
	stringstream out;
	out << ANSI_KWRD "HttpServer" ANSI_PUNCT "("
		<< "fd=" << server_fd << ANSI_PUNCT ", "
		<< "id=" << _id
		<< ANSI_PUNCT ")" ANSI_RST;
	return out.str();
}

HttpServer::operator string() const {
	return repr();
}

bool HttpServer::setup(const Config& conf) {
	config = conf;
	return setupSocket("127.0.0.1", 8080);  // Hardcoded for now
}

bool HttpServer::setupSocket(const std::string& ip, int port) {
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
	address.sin_port = htons(port);
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
	
	while (running) {
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