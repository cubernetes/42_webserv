#pragma once /* HttpServer.hpp */

#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <set>
#include <string>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/types.h>
#include <time.h>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "Constants.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

using Constants::MultPlexType;
using std::map;
using std::pair;
using std::set;
using std::string;
using std::vector;

class HttpServer {
  public:
    ~HttpServer();
    HttpServer(const string &configPath, Logger &log, size_t onlyCheckConfig = 0);
    operator string() const;

    // entry point, will run forever unless interrupted by signals or exceptions
    void run();

    //// forward decls ////
    struct Server;
    struct AddrPortCompare;
    struct HttpRequest;
    struct CgiProcess;
    enum FdState { FD_READABLE, FD_WRITEABLE, FD_OTHER_STATE };
    enum ChunkParsingState { PARSE_CHUNK, PARSE_CHUNK_SIZE };
    enum RequestState { READING_HEADERS, READING_BODY, REQUEST_COMPLETE, REQUEST_ERROR };

    //// typedefs ////
    typedef pair<struct in_addr, in_port_t_helper> AddrPort;
    typedef map<AddrPort, size_t, AddrPortCompare> DefaultServers;
    typedef vector<int> SelectFds;
    typedef vector<Server> Servers;
    typedef vector<struct pollfd> PollFds;
    typedef vector<struct epoll_event> EpollFds;
    typedef vector<FdState> FdStates;
    typedef map<string, string> MimeTypes;
    typedef map<string, string> Headers;
    typedef map<int, string> StatusTexts;
    typedef string PendingWrite;
    typedef map<int, PendingWrite> PendingWriteMap;
    typedef map<int, CgiProcess> ClientFdToCgiMap;
    typedef map<int, int> CgiFdToClientMap;
    typedef set<int> PendingCloses;
    typedef map<int, HttpRequest> PendingRequests;
    typedef map<int, std::time_t> PersistConns;

    //// structs and other constructs ////
    struct HttpRequest {
        string method;
        string path;
        string rawQuery;
        string httpVersion;
        Headers headers;
        string body;
        RequestState state;
        size_t contentLength;
        bool chunkedTransfer;
        size_t bytesRead;
        string temporaryBuffer;
        bool pathParsed;
        ChunkParsingState chunkParsingState;
        size_t thisChunkSize;

        HttpRequest()
            : method(), path("/"), rawQuery(), httpVersion(), headers(), body(), state(READING_HEADERS), contentLength(0),
              chunkedTransfer(false), bytesRead(0), temporaryBuffer(), pathParsed(false), chunkParsingState(PARSE_CHUNK_SIZE),
              thisChunkSize() {}
    };

    struct Server {        // Basically just a thin wrapper around ServerCtx, but with some
                           // fields transformed for ease of use and efficiency
        struct in_addr ip; // network byte order // Should multiple listen directive be possible in
                           // the future, then this, along with port, would become a vector of pairs,
                           // but for MVP's sake, let's keep it at one
        in_port_t port;    // network byte order
        const Directives &directives;
        const LocationCtxs &locations;
        const Arguments &serverNames;
        Logger &log;
        size_t id;

        ~Server();
        Server(const Directives &_directives, const LocationCtxs &_locations, const Arguments &_serverNames, Logger &_log, size_t _id);
        Server(const Server &other);
        Server &operator=(const Server &other);
    };
    struct AddrPortCompare {
        bool operator()(const AddrPort &a, const AddrPort &b) const {
            if (std::memcmp(&a.first, &b.first, sizeof(a.first)) < 0)
                return true;
            if (std::memcmp(&a.first, &b.first, sizeof(a.first)) > 0)
                return false;
            return a.second.port < b.second.port;
        }
    };
    struct MultPlexFds {
        MultPlexType multPlexType;

        fd_set selectFdSet;
        SelectFds selectFds;

        PollFds pollFds;

        EpollFds epollFds;

        FdStates fdStates;

        MultPlexFds(MultPlexType initMultPlexType)
            : multPlexType(initMultPlexType), selectFdSet(), selectFds(), pollFds(), epollFds(), fdStates() {
            FD_ZERO(&selectFdSet);
        }
        // private: //should be private, but can't because of Repr.hpp
        MultPlexFds() : multPlexType(Constants::defaultMultPlexType), selectFdSet(), selectFds(), pollFds(), epollFds(), fdStates() {}
    };
    struct CgiProcess {
        pid_t pid;
        int readFd;
        int writeFd;
        string response;
        size_t totalSize;
        int clientSocket;
        const LocationCtx *location;
        bool headersSent;
        std::time_t lastActive;
        bool dead;
        bool noRecentReadEvent;
        bool done;

        CgiProcess(pid_t _pid, int _readFd, int _writeFd, int _clientSocket, const LocationCtx *_location)
            : pid(_pid), readFd(_readFd), writeFd(_writeFd), response(), totalSize(0), clientSocket(_clientSocket), location(_location),
              headersSent(false), lastActive(std::time(NULL)), dead(false), noRecentReadEvent(true), done(false) {}
        CgiProcess(const CgiProcess &other)
            : pid(other.pid), readFd(other.readFd), writeFd(other.writeFd), response(other.response), totalSize(other.totalSize),
              clientSocket(other.clientSocket), location(other.location), headersSent(other.headersSent), lastActive(other.lastActive),
              dead(other.dead), noRecentReadEvent(other.noRecentReadEvent), done(other.done) {}
        CgiProcess &operator=(const CgiProcess &) {
            return *this;
        } // forgot the reason why we have define a copy constructor and copy assignment
          // operator, but I think it was because we use CgiProcess in a map and something
          // with references or so
    };

    const vector<int> &get_listeningSockets() const { return _listeningSockets; }
    const string &get_httpVersionString() const { return _httpVersionString; }
    const string &get_rawConfig() const { return _rawConfig; }
    const Config &get_config() const { return _config; }
    const MimeTypes &get_mimeTypes() const { return _mimeTypes; }
    const StatusTexts &get_statusTexts() const { return _statusTexts; }
    const PendingWriteMap &get_pendingWrites() const { return _pendingWrites; }
    const PendingCloses &get_pendingCloses() const { return _pendingCloses; }
    const Servers &get_servers() const { return _servers; }
    const DefaultServers &get_defaultServers() const { return _defaultServers; }
    const PendingRequests &get_pendingRequests() const { return _pendingRequests; }
    const MultPlexFds &get_monitorFds() const { return _monitorFds; }
    const ClientFdToCgiMap &get_clientToCgi() const { return _clientToCgi; }
    const CgiFdToClientMap &get_cgiToClient() const { return _cgiToClient; }

    static bool _running;
    MultPlexFds _monitorFds;
    ClientFdToCgiMap _clientToCgi;
    CgiFdToClientMap _cgiToClient;

  private:
    //// private members ////
    vector<int> _listeningSockets;
    const string _httpVersionString;
    const string _rawConfig;
    const Config _config;
    MimeTypes _mimeTypes;
    StatusTexts _statusTexts;
    PendingWriteMap _pendingWrites;
    PendingCloses _pendingCloses;
    Servers _servers;
    DefaultServers _defaultServers;
    PendingRequests _pendingRequests;
    set<int> _tmpCgiFds;
    PersistConns persistConns;
    Logger &log;

    //// private methods ////
    // HttpServer must always be constructed with a config
    HttpServer()
        : _monitorFds(Constants::defaultMultPlexType), _clientToCgi(), _cgiToClient(), _listeningSockets(), _httpVersionString(),
          _rawConfig(), _config(), _mimeTypes(), _statusTexts(), _pendingWrites(), _pendingCloses(), _servers(), _defaultServers(),
          _pendingRequests(), _tmpCgiFds(), persistConns(), log(Logger::lastInstance()) {};
    // Copying HttpServer is forbidden, since that would violate the 1-1 mapping between a
    // server and its config
    HttpServer(const HttpServer &)
        : _monitorFds(Constants::defaultMultPlexType), _clientToCgi(), _cgiToClient(), _listeningSockets(), _httpVersionString(),
          _rawConfig(), _config(), _mimeTypes(), _statusTexts(), _pendingWrites(), _pendingCloses(), _servers(), _defaultServers(),
          _pendingRequests(), _tmpCgiFds(), persistConns(), log(Logger::lastInstance()) {};
    // HttpServer cannot be assigned to
    HttpServer &operator=(const HttpServer &) { return *this; };

    // Setup
    void setupServers(const Config &config);
    void setupListeningSocket(const Server &server);
    void initSignals();

    // Adding a client
    void addNewClient(int listeningSocket);
    void addClientSocketToPollFds(MultPlexFds &monitorFds, int clientSocket);
    void addClientSocketToMonitorFds(MultPlexFds &monitorFds, int clientSocket);

    // Reading from a client
    void readFromClient(int clientSocket);

    // request parsing
    size_t findMatchingServer(const string &host, const struct in_addr &addr, in_port_t port) const;
    size_t findMatchingLocation(const Server &serverConfig, const string &path) const;
    size_t getIndexOfServerByHost(const string &requestedHost, const struct in_addr &addr, in_port_t port) const;
    size_t getIndexOfDefaultServer(const struct in_addr &addr, in_port_t port) const;
    const LocationCtx &requestToLocation(int clientSocket, const HttpRequest &request);
    struct sockaddr_in getSockaddrIn(int clientSocket);
    string canonicalizePath(const string &path);
    string percentDecode(const string &str);
    string resolveDots(const string &str);

    // request handling
    void handleRequest(int clientSocket, const HttpRequest &request, const LocationCtx &location);
    void handleRequestInternally(int clientSocket, const HttpRequest &request, const LocationCtx &location);
    bool methodAllowed(int clientSocket, const HttpRequest &request, const LocationCtx &location);
    void handleDelete(int clientSocket, const HttpRequest &request, const LocationCtx &location);
    void rewriteRequest(int clientSocket, int statusCode, const string &urlOrText, const LocationCtx &location);
    void redirectClient(int clientSocket, const string &newUri, int statusCode = 301);
    void handleUpload(int clientSocket, const HttpRequest &request, const LocationCtx &location, bool overwrite);

    // static file serving
    void serveStaticContent(int clientSocket, const HttpRequest &request, const LocationCtx &location);
    string determineDiskPath(const HttpRequest &request, const LocationCtx &location);
    bool handlePathWithoutSlash(int clientSocket, const string &diskPath, const HttpRequest &request, const LocationCtx &location,
                                bool sendErrorMsg = true);
    void handlePathWithSlash(int clientSocket, const string &diskPath, const HttpRequest &request, const LocationCtx &location,
                             bool sendErrorMsg = true);
    bool handleIndexes(int clientSocket, const string &diskPath, const HttpRequest &request, const LocationCtx &location);
    bool handleDirectoryRedirect(int clientSocket, const string &uri);

    // CGI
    bool requestIsForCgi(const HttpRequest &request, const LocationCtx &location);
    void handleCgiRead(int fd);

    // Timeout
    void checkForInactiveClients();

    // Writing to a client
    void writeToClient(int clientSocket);
    void sendString(int clientSocket, const string &payload, int statusCode = 200, const string &contentType = "text/html",
                    bool onlyHeaders = false);

  public:
    void sendError(int clientSocket, int statusCode, const LocationCtx *const location);
    void queueWrite(int clientSocket, const string &data);

  private:
    bool sendErrorPage(int clientSocket, int statusCode, const LocationCtx &location);
    bool sendFileContent(int clientSocket, const string &filePath, const LocationCtx &location, int statusCode = 200,
                         const string &contentType = "", bool onlyHeaders = false);
    PendingWrite &updatePendingWrite(PendingWrite &pw);

    // Removing a client
    void removeClient(int clientSocket);
    void closeAndRemoveMultPlexFd(MultPlexFds &monitorFds, int fd);
    void removePollFd(MultPlexFds &monitorFds, int fd);
    void closeAndRemoveAllMultPlexFd(MultPlexFds &monitorFds);
    void closeAndRemoveAllPollFd(MultPlexFds &monitorFds);
    bool maybeTerminateConnection(PendingWriteMap::iterator it, int clientSocket);
    void terminateIfNoPendingDataAndNoCgi(PendingWriteMap::iterator &it, int clientSocket, ssize_t bytesSent);
    void terminatePendingCloses(int clientSocket);

    // Monitor sockets (the blocking part)
    MultPlexFds getReadyFds(MultPlexFds &monitorFds);
    MultPlexFds doPoll(MultPlexFds &monitorFds);
    MultPlexFds getReadyPollFds(MultPlexFds &monitorFds, int nReady, struct pollfd *pollFds, nfds_t nPollFds);

    // Handle monitoring state for socket (i.e. for POLL, add/remove the POLLOUT event,
    // etc.)
    void stopMonitoringForWriteEvents(MultPlexFds &monitorFds, int clientSocket);
    void startMonitoringForWriteEvents(MultPlexFds &monitorFds, int clientSocket);
    void updatePollEvents(MultPlexFds &monitorFds, int clientSocket, short events, bool add);

    // Multiplex I/O file descriptor helpers
    void handleReadyFds(const MultPlexFds &readyFds);
    struct pollfd *multPlexFdsToPollFds(const MultPlexFds &fds);
    nfds_t getNumberOfPollFds(const MultPlexFds &fds);
    int multPlexFdToRawFd(const MultPlexFds &readyFds, size_t i);
    vector<int> multPlexFdsToRawFds(const MultPlexFds &readyFds);
    MultPlexFds determineRemoteClients(const MultPlexFds &m, vector<int> ls, const CgiFdToClientMap &cgiToClient);

    // helpers
    string getMimeType(const string &path);
    string statusTextFromCode(int statusCode);
    string wrapInHtmlBody(const string &text);
    bool isListeningSocket(int socket);

    // POST related
    bool isHeaderComplete(const HttpRequest &request) const;
    bool needsMoreData(const HttpRequest &request) const;
    void processContLenChunkedAndConnectionHeaders(int clientSocket, HttpRequest &request);
    bool validateRequest(const HttpRequest &request, int clientSocket);
    bool parseRequestLine(int clientSocket, const string &line, HttpRequest &request);
    bool parseHeader(const string &line, HttpRequest &request);
    size_t getRequestSizeLimit(int clientSocket, const HttpRequest &request);

    // Request processing stages
    void handleIncomingData(int clientSocket, const char *buffer, ssize_t bytesRead);
    bool processRequestHeaders(int clientSocket, HttpRequest &request, const string &rawData);
    bool processRequestBody(int clientSocket, HttpRequest &request, const char *buffer, size_t bytesRead);
    void finalizeRequest(int clientSocket, HttpRequest &request);
    void removeClientAndRequest(int clientSocket);
    bool checkRequestBodySize(int clientSocket, const HttpRequest &request, size_t currentSize);
    bool checkRequestSizeWithoutBody(int clientSocket, size_t currentSize);

    // chunked request
    bool processChunkedData(int clientSocket, HttpRequest &request);
    bool getChunkAndConsume(string &buffer, size_t chunkSize, string &retChunk);
    bool getChunkSizeStrAndConsume(string &buffer, string &chunkSizeStr);
};

std::ostream &operator<<(std::ostream &, const HttpServer &);

void initMimeTypes(HttpServer::MimeTypes &mimeTypes);
void initStatusTexts(HttpServer::StatusTexts &statusTexts);
