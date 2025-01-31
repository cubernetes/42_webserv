#include "HttpServer.hpp"

HttpServer::~HttpServer() {
	TRACE_DTOR;
	closeAndRemoveAllMultPlexFd(_monitorFds);
}

void finish(int signal) {
	(void)signal;
	Logger::logInfo("\nShutting down");
	HttpServer::_running = false;
}

void HttpServer::initSignals() {
	::signal(SIGINT, &finish);
	::signal(SIGTERM, &finish);
}

bool HttpServer::_running = true;
HttpServer::HttpServer(const string& configPath) : _monitorFds(Constants::defaultMultPlexType), _clientToCgi(), _cgiToClient(), _listeningSockets(), _pollFds(_monitorFds.pollFds), _httpVersionString(Constants::httpVersionString), _rawConfig(readConfig(configPath)), _config(parseConfig(_rawConfig)), _mimeTypes(), _statusTexts(), _pendingWrites(), _pendingCloses(), _servers(), _defaultServers(), _pendingRequests() {
	TRACE_ARG_CTOR(const string&, configPath);
	initSignals();
	initStatusTexts(_statusTexts);
	initMimeTypes(_mimeTypes);
	setupServers(_config);
}

HttpServer::operator string() const {
	return ::repr(*this);
}

std::ostream& operator<<(std::ostream& os, const HttpServer& httpServer) {
	return os << static_cast<string>(httpServer);
}

void HttpServer::run() {
	while (_running) {
		MultPlexFds readyFds = getReadyFds(_monitorFds); // block until: 1) new client
														 //              2) client that can be written to (response)
														 //              3) client that can be read from (request)
														 //              4) Constants::multiplexTimeout milliseconds is over
		handleReadyFds(readyFds); // 1) accept new client and add to pool
								  // 2) write pending data for Constants::chunkSize bytes (and remove this disposition if done writing)
								  // 3) read data for Constants::chunkSize bytes (or remove client if appropriate)
								  // 4) do nothing
		checkForInactiveClients();
	}
}
