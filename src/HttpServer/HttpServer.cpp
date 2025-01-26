#include "HttpServer.hpp"

HttpServer::~HttpServer() {
	TRACE_DTOR;
	closeAndRemoveAllMultPlexFd(_monitorFds);
}

HttpServer::HttpServer(const string& configPath) : _listeningSockets(), _monitorFds(Constants::defaultMultPlexType), _pollFds(_monitorFds.pollFds), _httpVersionString(Constants::httpVersionString), _rawConfig(readConfig(configPath)), _config(parseConfig(_rawConfig)), _mimeTypes(), _statusTexts(), _pendingWrites(), _pendingCloses(), _servers(), _defaultServers(), _cgiProcesses() {
	TRACE_ARG_CTOR(const string&, configPath);
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

void HttpServer::timeoutHandler(int clientSocket) {
	close(clientSocket);
	Logger::logError("Client socket " + STR(clientSocket) + " timed out and was closed.");
}

void HttpServer::updateClientActivity(int clientSocket) {
	_clientLastActivity[clientSocket] = time(NULL); // Update the last activity time
}

void HttpServer::checkForInactiveClients() {
	time_t currentTime = time(NULL);
	for (std::map<int, time_t>::iterator it = _clientLastActivity.begin(); it != _clientLastActivity.end(); ) {
		if (currentTime - it->second > CLIENT_TIMEOUT) {
			int clientSocket = it->first;
			timeoutHandler(clientSocket);
			removeClient(clientSocket);
			_clientLastActivity.erase(it++);
		} else
			++it;
	}
}

void HttpServer::run() {
	while (true) {
		MultPlexFds readyFds = getReadyFds(_monitorFds); // block until: 1) new client
														 //              2) client that can be written to (response)
														 //              3) client that can be read from (request)
														 //              4) Constants::multiplexTimeout milliseconds is over
		handleReadyFds(readyFds); // 1) accept new client and add to pool
								  // 2) write pending data for Constants::chunkSize bytes (and remove this disposition if done writing)
								  // 3) read data for Constants::chunkSize bytes (or remove client if appropriate)
								  // 4) do nothing
		checkForInactiveClients();
		usleep(100000); //or some random time idk but not to occupy da cpu
	}
	/* NOTREACHED */
}
