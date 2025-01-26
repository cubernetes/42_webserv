#include "HttpServer.hpp"

static string getServerIpStr(const ServerCtx& serverCtx) {
	const Arguments hostPort = getFirstDirective(serverCtx.first, "listen");
	return hostPort[0];
}

static struct in_addr getServerIp(const ServerCtx& serverCtx) {
	const Arguments hostPort = getFirstDirective(serverCtx.first, "listen");

	struct addrinfo hints, *res;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(hostPort[0].c_str(), NULL, &hints, &res) != 0) {
		throw runtime_error("Invalid IP address for listen directive in serverCtx config");
	}
	struct in_addr ipv4 = ((struct sockaddr_in*)res->ai_addr)->sin_addr;
	freeaddrinfo(res);
	return ipv4;
}

static in_port_t getServerPort(const ServerCtx& serverCtx) {
	const Arguments hostPort = getFirstDirective(serverCtx.first, "listen");
	return htons((in_port_t)std::atoi(hostPort[1].c_str()));
}

static int createTcpListenSocket() {
	int listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listeningSocket < 0)
		throw runtime_error("Failed to create socket");
	
	int opt = 1;
	if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(listeningSocket);
		throw runtime_error("Failed to set socket options");
	}
	return listeningSocket;
}

static void bindSocket(int listeningSocket, const HttpServer::Server& server) {
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = server.port;
	address.sin_addr = server.ip;
	
	if (bind(listeningSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
		close(listeningSocket);
		throw runtime_error(string("bind error: ") + strerror(errno));
	}
}

static void listenSocket(int listeningSocket) {
	if (listen(listeningSocket, SOMAXCONN) < 0) {
		close(listeningSocket);
		throw runtime_error(string("listen error: ") + strerror(errno));
	}
}

void HttpServer::setupListeningSocket(const Server& server) {
	int listeningSocket = createTcpListenSocket();
	
	bindSocket(listeningSocket, server);
	listenSocket(listeningSocket);
	
	struct pollfd pfd;
	pfd.fd = listeningSocket;
	pfd.events = POLLIN;
	_monitorFds.pollFds.push_back(pfd);

	_listeningSockets.push_back(listeningSocket);
}

void HttpServer::setupServers(const Config& config) {
	for (ServerCtxs::const_iterator serverCtx = config.second.begin(); serverCtx != config.second.end(); ++serverCtx) {
		Server server(
			serverCtx->first,
			serverCtx->second,
			getFirstDirective(serverCtx->first, "server_name")
		);

		server.ip = getServerIp(*serverCtx);
		server.port = getServerPort(*serverCtx);

		setupListeningSocket(server);
		cout << cmt("Server with names ") << repr(server.serverNames)
			<< cmt(" is listening on ") << num(getServerIpStr(*serverCtx))
			<< num(":") << repr(ntohs(server.port)) << '\n';
			
		_servers.push_back(server);

		AddrPort addr(server.ip, server.port);
		if (_defaultServers.find(addr) == _defaultServers.end())
			_defaultServers[addr] = _servers.size() - 1;
	}
}

