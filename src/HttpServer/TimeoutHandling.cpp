#include "HttpServer.hpp"

void HttpServer::checkForInactiveClients() {
	for (CgiProcessMap::iterator it = _cgiProcesses.begin(); it != _cgiProcesses.end(); ) {
		CgiProcess& process = it->second;
		process.pollCycles++;

		int cyclesForTimeout = CGI_TIMEOUT * (1000 / Constants::multiplexTimeout);
		if (process.pollCycles > cyclesForTimeout) {
			kill(process.pid, SIGKILL);
			waitpid(process.pid, NULL, 0);
			sendError(process.clientSocket, 504, process.location);
			
			closeAndRemoveMultPlexFd(_monitorFds, it->first);
			std::map<int, CgiProcess>::iterator tmp = it;
			++it;
			_cgiProcesses.erase(tmp);
		} else {
			++it;
		}
	}
}
