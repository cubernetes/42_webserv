#pragma once /* HttpServer.hpp */

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <map>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ostream>
#include <set> 
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <vector>
#include <unistd.h>

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
	int							_serverFd;
	std::vector<struct pollfd>	_pollFds;
	bool						_running;
	Config						_config;

	// Instance tracking
	unsigned int				_id;
	static unsigned int			_idCntr;
	std::map<string,string>		_mimeTypes;
	struct HttpRequest {
		string						method;
		string						path;
		string						httpVersion;
		std::map<string, string>	headers;

		HttpRequest() :
			method(),
			path(),
			httpVersion(),
			headers() {}
	};

	struct PendingWrite {
		string	data;
		size_t	bytesSent;

		PendingWrite() : data(), bytesSent(0) {}
		PendingWrite(const string& d) : data(d), bytesSent(0) {}
	};

	std::map<int, PendingWrite>	_pendingWrites;
	std::set<int>				_pendingClose;
	struct ServerConfig {
		struct in_addr	ip;
		int				port;
		std::vector<std::string> serverNames;
		Directives directives;
		LocationCtxs locations;

		ServerConfig() : ip(), port(0), serverNames(), directives(), locations() {
			ip.s_addr = 0;
		}
	};

	struct AddrPortCompare {
	bool operator()(const std::pair<struct in_addr, int>& a,
					const std::pair<struct in_addr, int>& b) const {
		if (memcmp(&a.first, &b.first, sizeof(struct in_addr)) < 0)
			return true;
		if (memcmp(&a.first, &b.first, sizeof(struct in_addr)) > 0)
			return false;
		return a.second < b.second;
	}
};

	std::vector<ServerConfig> _servers;
	std::map<std::pair<struct in_addr, int>, const ServerConfig*, AddrPortCompare> _defaultServers;
	bool setupSocket(const std::string& ip, int port);
	void handleNewConnection();
	void handleClientData(int clientFd);
	void closeConnection(int clientFd);
	void removePollFd(int fd);
	HttpRequest parseHttpRequest(const char *buffer);
	bool validateServerConfig(int clientFd, const ServerCtx& serverConfig, string& rootDir, string& defaultIndex);
	bool validatePath(int clientFd, const string& path);
	bool handleDirectoryRedirect(int clientFd, const HttpRequest& request, string& filePath, 
									const string& defaultIndex, struct stat& fileStat);
	void sendFileContent(int clientFd, const string& filePath);
	void handleGetRequest(int clientFd, const HttpRequest& request);
	void sendError(int clientFd, int statusCode, const string& statusText);
	void sendText(int clientFd, const string& text);
	void initMimeTypes();
	string getMimeType(const string& path);
	void queueWrite(int clientFd, const string& data);
	void handleClientWrite(int clientFd);
	const ServerConfig* findMatchingServer(const std::string& host, const struct in_addr& addr, int port) const;
};

// global scope swap (aka ::swap), needed since friend keyword is forbidden :(
void swap(HttpServer&, HttpServer&) /* noexcept */;
std::ostream& operator<<(std::ostream&, const HttpServer&);
