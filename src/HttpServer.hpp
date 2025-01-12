#pragma once

#include <vector>
#include <map>
#include <string>
#include <sys/poll.h>
#include "Config.hpp"
#include "Logger.hpp"

class HttpServer {
public:
	// Orthodox Canonical Form requirements
	~HttpServer();
	HttpServer();
	HttpServer(const HttpServer&);
	HttpServer& operator=(HttpServer);
	
	// Core functionality
	bool setup(const Config& config);
	void run();

	// Required for repr
	void swap(HttpServer&);
	string repr() const;
	operator string() const;

private:
	int server_fd;
	std::vector<struct pollfd> poll_fds;
	bool running;
	Config config;
	
	// Instance tracking
	unsigned int _id;
	static unsigned int _idCntr;

	// Helper methods
	bool setupSocket(const std::string& ip, int port);
	void handleNewConnection();
	void handleClientData(int client_fd);
	void closeConnection(int client_fd);
	void removePollFd(int fd);
};

void swap(HttpServer&, HttpServer&);
std::ostream& operator<<(std::ostream&, const HttpServer&);