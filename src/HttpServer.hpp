#pragma once /* HttpServer.hpp */

#include <algorithm>
#include <cctype>
#include <vector>
#include <map>
#include <string>
#include <sys/poll.h>

#include "Config.hpp"

using std::string;

class HttpServer {
public:
	// Orthodox Canonical Form requirements
	~HttpServer();
	HttpServer();
	HttpServer(const HttpServer&);
	HttpServer& operator=(HttpServer);
	void swap(HttpServer&); // copy swap idiom

	// string conversion
	operator string() const;
	
	// Core functionality
	bool setup(const Config& config);
	void run();

	// Getters
	int get_serverFd() const;
	const std::vector<struct pollfd>& get_pollFds() const;
	bool get_running() const;
	const Config& get_config() const;
	unsigned int get_id() const;

private:
	int _serverFd;
	std::vector<struct pollfd> _pollFds;
	bool _running;
	Config _config;

	// Instance tracking
	unsigned int _id;
	static unsigned int _idCntr;
	std::map<string,string> _mimeTypes;
	struct HttpRequest {
		string method;
		string path;
		string httpVersion;
		std::map<string, string> headers;

		HttpRequest() :
			method(),
			path(),
			httpVersion(),
			headers() {}
	};

	bool setupSocket(const std::string& ip, int port);
	void handleNewConnection();
	void handleClientData(int clientFd);
	void closeConnection(int clientFd);
	void removePollFd(int fd);
	HttpRequest parseHttpRequest(const char *buffer);
	void handleGetRequest(int clientFd, const HttpRequest& request);
	void sendError(int clientFd, int statusCode, const string& statusText);
	void initMimeTypes();
	string getMimeType(const string& path);
};

// global scope swap (aka ::swap), needed since friend keyword is forbidden :(
void swap(HttpServer&, HttpServer&) /* noexcept */;
std::ostream& operator<<(std::ostream&, const HttpServer&);
