#include "HttpServer.hpp"

void HttpServer::checkForInactiveClients() {
	for (std::map<int, CGIProcess>::iterator it = _cgiProcesses.begin(); it != _cgiProcesses.end(); ) {
		CGIProcess& process = it->second;
		process.pollCycles++;

		int cyclesForTimeout = CGI_TIMEOUT * (1000 / Constants::multiplexTimeout);
		if (process.pollCycles > cyclesForTimeout) {
			kill(process.pid, SIGKILL);
			waitpid(process.pid, NULL, 0);
			sendError(process.clientSocket, 504, process.location);
			
			closeAndRemoveMultPlexFd(_monitorFds, it->first);
			std::map<int, CGIProcess>::iterator tmp = it;
			++it;
			_cgiProcesses.erase(tmp);
		} else {
			++it;
		}
	}

	time_t currentTime = std::time(NULL);
	for (UploadStates::iterator it = _uploadStates.begin(); it != _uploadStates.end();) {
		if (currentTime - it->second.lastActive > UPLOAD_TIMEOUT) {
			cleanupUpload(it->first);
			it = _uploadStates.begin();
		} else {
			++it;
		}
	}
}