#include "HttpServer.hpp"
#include <sys/stat.h>
#include <sstream>

std::string HttpServer::generateTempFilename(const std::string& prefix) {
	std::stringstream ss;
	ss << "/tmp/" << prefix << time(NULL) << "_" << rand();
	return ss.str();
}

void HttpServer::initializeUpload(int clientSocket, const HttpRequest& request) {
	UploadState uploadState;
	uploadState.tempFilePath = generateTempFilename();
	
	uploadState.tempFile = new std::ofstream(uploadState.tempFilePath.c_str(), std::ios::binary);
	
	if (!uploadState.tempFile || !*uploadState.tempFile) {
		if (uploadState.tempFile) {
			delete uploadState.tempFile;
		}
		sendError(clientSocket, 500, NULL);
		return;
	}
	
	_uploadStates[clientSocket] = uploadState;
}

void HttpServer::handleUploadData(int clientSocket, const char* buffer, size_t bytesRead) {
	UploadStates::iterator it = _uploadStates.find(clientSocket);
	if (it == _uploadStates.end()) {
		return;
	}
	
	UploadState& state = it->second;
	
	// Check size limit
	if (state.bytesWritten + bytesRead > MAX_UPLOAD_SIZE) {
		cleanupUpload(clientSocket);
		sendError(clientSocket, 413, NULL); // Payload Too Large
		return;
	}
	
	// Write data to temp file
	state.tempFile->write(buffer, bytesRead);
	state.bytesWritten += bytesRead;
	state.lastActive = time(NULL);
}

bool HttpServer::validateUploadDir(const std::string& path) {
	struct stat st;
	if (stat(path.c_str(), &st) != 0) {
		return false;
	}
	return S_ISDIR(st.st_mode) && (st.st_mode & S_IWUSR);
}

void HttpServer::createUploadDirectories(const std::string& path) {
	size_t pos = 0;
	while ((pos = path.find('/', pos + 1)) != std::string::npos) {
		std::string subPath = path.substr(0, pos);
		mkdir(subPath.c_str(), 0755);
	}
}

void HttpServer::cleanupUpload(int clientSocket, bool success) {
	UploadStates::iterator it = _uploadStates.find(clientSocket);
	if (it == _uploadStates.end()) {
		return;
	}
	
	UploadState& state = it->second;
	state.tempFile->close();
	
	if (!success) {
		unlink(state.tempFilePath.c_str());
	}
	
	_uploadStates.erase(it);
}

void HttpServer::finalizeUpload(int clientSocket, HttpRequest& request) {
	UploadStates::iterator it = _uploadStates.find(clientSocket);
	if (it == _uploadStates.end()) {
		sendError(clientSocket, 500, NULL);
		return;
	}
	
	UploadState& state = it->second;
	state.tempFile->close();
	
	try {
		const LocationCtx& location = requestToLocation(clientSocket, request);
		if (!directiveExists(location.second, "upload_dir")) {
			cleanupUpload(clientSocket);
			sendError(clientSocket, 403, &location);
			return;
		}
		
		std::string uploadDir = getFirstDirective(location.second, "upload_dir")[0];
		if (!validateUploadDir(uploadDir)) {
			createUploadDirectories(uploadDir);
			if (!validateUploadDir(uploadDir)) {
				cleanupUpload(clientSocket);
				sendError(clientSocket, 500, &location);
				return;
			}
		}
		
		// Generate final filename
		std::string filename = request.headers["Content-Disposition"];
		if (filename.empty()) {
			filename = "upload_" + STR(time(NULL));
		}
		std::string finalPath = uploadDir + "/" + filename;
		
		// Move temp file to final location
		if (rename(state.tempFilePath.c_str(), finalPath.c_str()) != 0) {
			cleanupUpload(clientSocket);
			sendError(clientSocket, 500, &location);
			return;
		}
		
		cleanupUpload(clientSocket, true);
		sendString(clientSocket, "File uploaded successfully\n", 201);
		
	} catch (const std::exception& e) {
		cleanupUpload(clientSocket);
		Logger::logError(std::string("Upload failed: ") + e.what());
		sendError(clientSocket, 500, NULL);
	}
}

void HttpServer::handlePost(int clientSocket, const HttpRequest& request, const LocationCtx& location) {
	if (!directiveExists(location.second, "upload_dir")) {
		sendError(clientSocket, 403, &location);
		return;
	}
	
	// Initialize file upload if needed
	if (_uploadStates.find(clientSocket) == _uploadStates.end()) {
		initializeUpload(clientSocket, request);
	}
	
	// Process body data
	if (!request.body.empty()) {
		handleUploadData(clientSocket, request.body.c_str(), request.body.size());
	}
	
	// If request is complete, finalize upload
	if (request.state == REQUEST_COMPLETE) {
		finalizeUpload(clientSocket, const_cast<HttpRequest&>(request));
	}
}