#pragma once /* HttpServer.hpp */

#include <algorithm>
#include <cctype>
#include <map>
#include <set> 
#include <string>
#include <vector>
#include <sys/poll.h>
#include <netinet/in.h>
#include <netdb.h>

#include "Config.hpp"

using std::string;

static inline bool operator<(const in_addr& lhs, const in_addr& rhs) {
	return lhs.s_addr < rhs.s_addr;
}

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
	const std::vector<int>& get_serverFds() const;
	const std::vector<struct pollfd>& get_pollFds() const;
	bool get_running() const;
	const Config& get_config() const;
	unsigned int get_id() const;

private:
	std::vector<int>			_serverFds;
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
		in_port_t			port;
		std::vector<std::string> serverNames;
		Directives directives;
		LocationCtxs locations;

		ServerConfig() : ip(), port(0), serverNames(), directives(), locations() {}
	};

	std::vector<ServerConfig> _servers;
	std::map<std::pair<struct in_addr, in_port_t>, ServerConfig> _defaultServers;

	bool setupSocket(const struct in_addr& ip, in_port_t port);
	void handleNewConnection(int serverFd);
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
	const ServerConfig* findMatchingServer(const std::string& host, const struct in_addr& ip, in_port_t port) const;

	bool isServerFd(int serverFd);
};

// global scope swap (aka ::swap), needed since friend keyword is forbidden :(
void swap(HttpServer&, HttpServer&) /* noexcept */;
std::ostream& operator<<(std::ostream&, const HttpServer&);
