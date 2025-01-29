#pragma once /* HttpServer.hpp */

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits.h>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <ostream>
#include <set> 
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "Constants.hpp"
#include "Config.hpp"
#include "Utils.hpp"
#include "DirectoryIndexing.hpp"

using Constants::MultPlexType;
using Constants::EPOLL;
using Constants::POLL;
using Constants::SELECT;
using Utils::STR;
using std::cout;
using std::map;
using std::pair;
using std::runtime_error;
using std::set;
using std::string;
using std::vector;

class HttpServer {
public:
	~HttpServer();
	HttpServer(const string& configPath);
	operator string() const;

	// entry point, will run forever unless interrupted by signals or exceptions
	void run();

	//// forward decls ////
	struct Server;
	struct AddrPortCompare;
	struct PendingWrite;
	enum FdState {
		FD_READABLE,
		FD_WRITEABLE,
		FD_OTHER_STATE,
	};

	//// typedefs ////
	typedef pair<struct in_addr, in_port_t> AddrPort;
	typedef map<AddrPort, size_t, AddrPortCompare> DefaultServers;
	typedef vector<int> SelectFds;
	typedef vector<Server> Servers;
	typedef vector<struct pollfd> PollFds;
	typedef vector<struct epoll_event> EpollFds;
	typedef vector<FdState> FdStates;
	typedef map<string, string> MimeTypes;
	typedef map<int, string> StatusTexts;
	typedef map<int, PendingWrite> PendingWrites;
	typedef set<int> PendingCloses;

	//// structs and other constructs ////
	struct HttpRequest {
		string				method;
		string				path;
		string				httpVersion;
		map<string, string>	headers;

		HttpRequest() :
			method(),
			path(),
			httpVersion(),
			headers() {}
	};
	struct PendingWrite {
		string	data;
		size_t	bytesSent;

		PendingWrite() : data(), bytesSent() {}
		PendingWrite(const string& d) : data(d), bytesSent() {}
	};
	struct Server { // Basically just a thin wrapper around ServerCtx, but with some fields transformed for ease of use and efficiency
		struct in_addr	ip; // network byte order // Should multiple listen directive be possible in the future, then this, along with port, would become a vector of pairs, but for MVP's sake, let's keep it at one
		in_port_t		port; // network byte order
		const Directives& directives;
		const LocationCtxs& locations;
		const Arguments& serverNames;

		Server(const Directives& _directives, const LocationCtxs& _locations, const Arguments& _serverNames) : ip(), port(), directives(_directives), locations(_locations), serverNames(_serverNames) {}
		Server(const Server& other) : ip(other.ip), port(other.port), directives(other.directives), locations(other.locations), serverNames(other.serverNames) {}
		Server& operator=(const Server& other) { (void)other; return *this; }
	};
	struct AddrPortCompare {
		bool operator()(const AddrPort& a,
						const AddrPort& b) const {
			if (memcmp(&a.first, &b.first, sizeof(a.first)) < 0)
				return true;
			if (memcmp(&a.first, &b.first, sizeof(a.first)) > 0)
				return false;
			return a.second < b.second;
		}
	};
	struct MultPlexFds {
		MultPlexType multPlexType;

		fd_set selectFdSet;
		SelectFds selectFds;

		PollFds pollFds;

		EpollFds epollFds;

		FdStates fdStates;

		MultPlexFds(MultPlexType initMultPlexType) : multPlexType(initMultPlexType), selectFdSet(), selectFds(), pollFds(), epollFds(), fdStates() { FD_ZERO(&selectFdSet); }
	//private:
		MultPlexFds() : multPlexType(Constants::defaultMultPlexType), selectFdSet(), selectFds(), pollFds(), epollFds(), fdStates() {}
	};

	struct CGIProcess {
		pid_t pid;
		int pipe_fd;
		string response;
		unsigned long totalSize;
		int clientSocket;
		const LocationCtx* location;
		bool headersSent;
		int pollCycles;
		
		CGIProcess(pid_t p, int fd, int client, const LocationCtx* loc) : 
			pid(p), pipe_fd(fd), response(), totalSize(0), clientSocket(client), location(loc), headersSent(false), pollCycles(0) {}
		CGIProcess(const CGIProcess& other) : pid(other.pid), pipe_fd(other.pipe_fd), response(other.response), totalSize(other.totalSize), clientSocket(other.clientSocket), location(other.location), headersSent(other.headersSent), pollCycles(other.pollCycles) { (void)other; }
		CGIProcess& operator=(const CGIProcess&) { return *this; }
};

	const vector<int>&				get_listeningSockets()	const { return _listeningSockets; }
	const MultPlexFds&				get_monitorFds()		const { return _monitorFds; }
	const PollFds&					get_pollFds()			const { return _pollFds; }
	const string&					get_httpVersionString()	const { return _httpVersionString; }
	const string&					get_rawConfig()			const { return _rawConfig; }
	const Config&					get_config()			const { return _config; }
	const MimeTypes&				get_mimeTypes()			const { return _mimeTypes; }
	const StatusTexts&				get_statusTexts()		const { return _statusTexts; }
	const PendingWrites&			get_pendingWrites()		const { return _pendingWrites; }
	const PendingCloses&			get_pendingCloses()		const { return _pendingCloses; }
	const Servers&					get_servers()			const { return _servers; }
	const DefaultServers&			get_defaultServers()	const { return _defaultServers; }
	map<int, CGIProcess>& 			get_CGIProcesses()		{ return _cgiProcesses; }
	MultPlexFds&					get_MonitorFds()		{ return _monitorFds; }
	void sendError(int clientSocket, int statusCode, const LocationCtx *const location);

private:
	//// private members ////
	vector<int>				_listeningSockets;
	MultPlexFds				_monitorFds;
	PollFds&				_pollFds;
	const string			_httpVersionString;
	const string			_rawConfig;
	const Config			_config;
	MimeTypes				_mimeTypes;
	StatusTexts				_statusTexts;
	PendingWrites			_pendingWrites;
	PendingCloses			_pendingCloses;
	Servers					_servers;
	DefaultServers			_defaultServers;
	map<int, CGIProcess>	_cgiProcesses;
	static const int		CGI_TIMEOUT = 5; 


	//// private methods ////
	// HttpServer must always be constructed with a config
	HttpServer() : _listeningSockets(), _monitorFds(Constants::defaultMultPlexType), _pollFds(_monitorFds.pollFds), _httpVersionString(), _rawConfig(), _config(), _mimeTypes(), _statusTexts(), _pendingWrites(), _pendingCloses(), _servers(), _defaultServers(), _cgiProcesses() {};
	// Copying HttpServer is forbidden, since that would violate the 1-1 mapping between a server and its config
	HttpServer(const HttpServer&) : _listeningSockets(), _monitorFds(Constants::defaultMultPlexType), _pollFds(_monitorFds.pollFds), _httpVersionString(), _rawConfig(), _config(), _mimeTypes(), _statusTexts(), _pendingWrites(), _pendingCloses(), _servers(), _defaultServers(), _cgiProcesses() {};
	// HttpServer cannot be assigned to
	HttpServer& operator=(const HttpServer&) { return *this; };

	// Setup
	void setupServers(const Config& config);
	void setupListeningSocket(const Server& server);

	// Adding a client
	void addNewClient(int listeningSocket);
	void addClientSocketToPollFds(MultPlexFds& monitorFds, int clientSocket);
	void addClientSocketToMonitorFds(MultPlexFds& monitorFds, int clientSocket);

	// Reading from a client
	void readFromClient(int clientSocket);
	ssize_t recvToBuffer(int clientSocket, char *buffer, size_t bufSiz);

	// request parsing
	HttpRequest parseHttpRequest(const char *buffer);
	void parseHeaders(std::istringstream& requestStream, string& line, HttpRequest& request);
	size_t findMatchingServer(const string& host, const struct in_addr& addr, in_port_t port) const;
	size_t findMatchingLocation(const Server& serverConfig, const string& path) const;
	size_t getIndexOfServerByHost(const string& requestedHost, const struct in_addr& addr, in_port_t port) const;
	size_t getIndexOfDefaultServer(const struct in_addr& addr, in_port_t port) const;
	const LocationCtx& requestToLocation(int clientSocket, const HttpRequest& request);
	struct sockaddr_in getSockaddrIn(int clientSocket);
	string canonicalizePath(const string& path);
	string percentDecode(const string& str);
	string resolveDots(const string& str);

	// request handling
	void handleRequest(int clientSocket, const HttpRequest& request, const LocationCtx& location);
	void handleRequestInternally(int clientSocket, const HttpRequest& request, const LocationCtx& location);
	bool methodAllowed(const HttpRequest& request, const LocationCtx& location);
	void handleDelete(int clientSocket, const HttpRequest& request, const LocationCtx& location);
	void rewriteRequest(int clientSocket, int statusCode, const string& urlOrText, const LocationCtx& location);
	void redirectClient(int clientSocket, const string& newUri, int statusCode = 301);
	
	// static file serving
	void serveStaticContent(int clientSocket, const HttpRequest& request, const LocationCtx& location);
	string determineDiskPath(const HttpRequest& request, const LocationCtx& location);
	bool handleUriWithoutSlash(int clientSocket, const string& diskPath, const HttpRequest& request, const LocationCtx& location, bool sendErrorMsg = true);
	void handleUriWithSlash(int clientSocket, const string& diskPath, const HttpRequest& request, const LocationCtx& location, bool sendErrorMsg = true);
	bool handleIndexes(int clientSocket, const string& diskPath, const HttpRequest& request, const LocationCtx& location);
	bool handleDirectoryRedirect(int clientSocket, const string& uri);

	// CGI
	bool requestIsForCgi(const HttpRequest& request, const LocationCtx& location);
	void handleCGIRead(int fd);

	// Timeout
	void checkForInactiveClients();
	void timeoutHandler(int clientSocket);

	// Writing to a client
	void writeToClient(int clientSocket);
	void queueWrite(int clientSocket, const string& data);
	void sendString(int clientSocket, const string& payload, int statusCode = 200, const string& contentType = "text/html");
	bool sendErrorPage(int clientSocket, int statusCode, const LocationCtx& location);
	bool sendFileContent(int clientSocket, const string& filePath, const LocationCtx& location, int statusCode = 200, const string& contentType = "");
	PendingWrite& updatePendingWrite(PendingWrite& pw);

	// Removing a client
	void removeClient(int clientSocket);
	void closeAndRemoveMultPlexFd(MultPlexFds& monitorFds, int fd);
	void removePollFd(MultPlexFds& monitorFds, int fd);
	void closeAndRemoveAllMultPlexFd(MultPlexFds& monitorFds);
	void closeAndRemoveAllPollFd(MultPlexFds& monitorFds);
	bool maybeTerminateConnection(PendingWrites::iterator it, int clientSocket);
	void terminateIfNoPendingData(PendingWrites::iterator& it, int clientSocket, ssize_t bytesSent);
	void terminatePendingCloses(int clientSocket);

	// Monitor sockets (the blocking part)
	MultPlexFds getReadyFds(MultPlexFds& monitorFds);
	MultPlexFds doPoll(MultPlexFds& monitorFds);
	MultPlexFds getReadyPollFds(MultPlexFds& monitorFds, int nReady, struct pollfd *pollFds, nfds_t nPollFds);

	// Handle monitoring state for socket (i.e. for POLL, add/remove the POLLOUT event, etc.)
	void stopMonitoringForWriteEvents(MultPlexFds& monitorFds, int clientSocket);
	void startMonitoringForWriteEvents(MultPlexFds& monitorFds, int clientSocket);
	void updatePollEvents(MultPlexFds& monitorFds, int clientSocket, short events, bool add);

	// Multiplex I/O file descriptor helpers
	void handleReadyFds(const MultPlexFds& readyFds);
	struct pollfd *multPlexFdsToPollFds(const MultPlexFds& fds);
	nfds_t getNumberOfPollFds(const MultPlexFds& fds);
	int multPlexFdToRawFd(const MultPlexFds& readyFds, size_t i);

	// helpers
	string getMimeType(const string& path);
	string statusTextFromCode(int statusCode);
	string wrapInHtmlBody(const string& text);
	bool isListeningSocket(int socket);
};

std::ostream& operator<<(std::ostream&, const HttpServer&);

void initMimeTypes(HttpServer::MimeTypes& mimeTypes);
void initStatusTexts(HttpServer::StatusTexts& statusTexts);

#include "Logger.hpp"
