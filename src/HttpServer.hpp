#pragma once

#include <vector>
#include <map>
#include <string>
#include <sys/poll.h>

#include "conf.hpp"

using std::string;

class HttpServer {
public:
	// Orthodox Canonical Form requirements
	~HttpServer();
	HttpServer();
	HttpServer(const HttpServer&);
	HttpServer& operator=(HttpServer);
	
	// Core functionality
	bool setup(const t_config& config);
	void run();

	void swap(HttpServer&);
	operator string() const;

	int get_server_fd() const;
	const std::vector<struct pollfd>& get_poll_fds() const;
	bool get_running() const;
	const t_config& get_config() const;
	unsigned int get_id() const;
private:
	int _server_fd;
	std::vector<struct pollfd> _poll_fds;
	bool _running;
	t_config _config;
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
