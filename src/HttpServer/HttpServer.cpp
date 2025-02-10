#include <cstddef>
#include <iostream>
#include <ostream>
#include <signal.h>
#include <unistd.h>

#include "Config.hpp"
#include "Constants.hpp"
#include "Exceptions.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Repr.hpp"

HttpServer::~HttpServer() {
    TRACE_DTOR;
    closeAndRemoveAllMultPlexFd(_monitorFds);
}

void cgiDied(int signal) {
    (void)signal;
    if (Logger::lastInstance().isdebug())
        write(1, "Some CGI process died\n", 22);
    ;
}

void finish(int signal) {
    (void)signal;
    HttpServer::_running = false;
}

void HttpServer::initSignals() {
    log.debug() << "Ignoring SIGPIPE signal (in case CGI process strikes)" << std::endl;
    ::signal(SIGPIPE,
             SIG_IGN); // writing to dead cgi -> sigpipe -> don't care just continue
    log.debug() << "Handling SIGINT to finish gracefully" << std::endl;
    ::signal(SIGINT, &finish);
    log.debug() << "Handling SIGTERM to finish gracefully" << std::endl;
    ::signal(SIGTERM, &finish);
    ::signal(SIGCHLD, &cgiDied);
}

bool HttpServer::_running = true;
HttpServer::HttpServer(const string &configPath, Logger &_log, size_t onlyCheckConfig)
    : _monitorFds(Constants::defaultMultPlexType), _clientToCgi(), _cgiToClient(), _listeningSockets(),
      _httpVersionString(Constants::httpVersionString), _rawConfig(removeComments(readConfig(configPath))),
      _config(parseConfig(_rawConfig)), _mimeTypes(), _statusTexts(), _pendingWrites(), _pendingCloses(), _servers(), _defaultServers(),
      _pendingRequests(), log(_log) {
    TRACE_ARG_CTOR(const string &, configPath);
    if (onlyCheckConfig > 0) {
        if (onlyCheckConfig > 1)
            std::cout << _rawConfig;
        std::cerr << "Config valid" << std::endl;
        throw OnlyCheckConfigException();
    }
    initSignals();
    initStatusTexts(_statusTexts);
    initMimeTypes(_mimeTypes);
    setupServers(_config);
}

HttpServer::operator string() const { return ::repr(*this); }

std::ostream &operator<<(std::ostream &os, const HttpServer &httpServer) { return os << static_cast<string>(httpServer); }

HttpServer::Server::~Server() { TRACE_DTOR; }
HttpServer::Server::Server(const Directives &_directives, const LocationCtxs &_locations, const Arguments &_serverNames, Logger &_log,
                           size_t _id)
    : ip(), port(), directives(_directives), locations(_locations), serverNames(_serverNames), log(_log), id(_id) {
    TRACE_ARG_CTOR(const Directives, _directives, const LocationCtxs, _locations, const Arguments, _serverNames);
}
HttpServer::Server::Server(const Server &other)
    : ip(other.ip), port(other.port), directives(other.directives), locations(other.locations), serverNames(other.serverNames),
      log(other.log), id(other.id) {
    TRACE_COPY_CTOR;
}
HttpServer::Server &HttpServer::Server::operator=(const Server &other) {
    TRACE_COPY_ASSIGN_OP;
    (void)other;
    return *this;
}

// clang-format off
void HttpServer::run() {
    log.debug() << "Starting mainloop" << std::endl;
	while (_running) {
		MultPlexFds readyFds = getReadyFds(_monitorFds); // block until: 1) new client
														 //              2) client that can be written to (response)
														 //              3) client that can be read from (request)
														 //              4) Constants::multiplexTimeout milliseconds is over
		handleReadyFds(readyFds); // 1) accept new client and add to pool
								  // 2) write pending data for Constants::chunkSize bytes (and remove this disposition if done writing)
								  // 3) read data for Constants::chunkSize bytes (or remove client if appropriate)
								  // 4) do nothing
		checkForInactiveClients(); // reset connection for timed out CGI's
		log.debug() << "Finished one mainloop iteration, the following line is intentionally left blank\n" << std::endl;
	}
  log.warn() << "Shutting server down" << std::endl;
}
// clang-format on
