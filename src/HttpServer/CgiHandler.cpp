#include "CgiHandler.hpp"
#include "ReprCgi.hpp"

using std::swap;
using std::string;

// De- & Constructors
CgiHandler::~CgiHandler() {
	TRACE_DTOR;
}

CgiHandler::CgiHandler(HttpServer& server, const string& extension, const string& program) 
	: _server(server), _extension(extension), _program(program) {
	TRACE_ARG_CTOR(string, extension, string, program);
}

CgiHandler::CgiHandler(const CgiHandler& other) :
	_server(other._server),
	_extension(other._extension),
	_program(other._program) {
	TRACE_COPY_CTOR;
}

// copy swap idiom
CgiHandler& CgiHandler::operator=(CgiHandler other) /* noexcept */ {
	TRACE_COPY_ASSIGN_OP;
	::swap(*this, other);
	return *this;
}

// Getters
const string& CgiHandler::get_extension() const { return _extension; }
const string& CgiHandler::get_program() const { return _program; }

void CgiHandler::swap(CgiHandler& other) /* noexcept */ {
	TRACE_SWAP_BEGIN;
	::swap(_extension, other._extension);
	::swap(_program, other._program);
	TRACE_SWAP_END;
}

CgiHandler::operator string() const { return ::repr(*this); }

void swap(CgiHandler& a, CgiHandler& b) /* noexcept */ {
	a.swap(b);
}

std::ostream& operator<<(std::ostream& os, const CgiHandler& other) {
	return os << static_cast<string>(other);
}

bool CgiHandler::canHandle(const string& path) const {
	return path.length() > _extension.length() && 
		   path.substr(path.length() - _extension.length()) == _extension;
}

std::map<string, string> CgiHandler::setupEnvironment(const HttpServer::HttpRequest& request,
													const LocationCtx& location) {
	std::map<string, string> env;

	string rootPath = getFirstDirective(location.second, "root")[0];
	string relativePath = request.path;
	if (Utils::isPrefix(rootPath, request.path)) {
		relativePath = request.path.substr(rootPath.length());
	}

	// Find any path info after the script name
	size_t scriptEnd = request.path.rfind(".py");
	if (scriptEnd != string::npos) {
		scriptEnd += 3; // TODO: @all decide yay or nay
		env["PATH_INFO"] = (scriptEnd < request.path.length()) ? request.path.substr(scriptEnd) : "";
	} else {
		env["PATH_INFO"] = "";
	}
	
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["SERVER_SOFTWARE"] = "webserv/1.0";
	env["REQUEST_METHOD"] = request.method;
	env["SCRIPT_FILENAME"] = rootPath + relativePath;
	env["QUERY_STRING"] = "";
	
	size_t queryPos = request.path.find('?');
	if (queryPos != string::npos) {
		env["QUERY_STRING"] = request.path.substr(queryPos + 1);
	}

	for (std::map<string, string>::const_iterator it = request.headers.begin(); 
		 it != request.headers.end(); ++it) {
		string headerName = "HTTP_" + it->first;
		std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::toupper);
		std::replace(headerName.begin(), headerName.end(), '-', '_');
		env[headerName] = it->second;
	}

	if (request.method == "POST") {
		if (request.headers.find("Content-Length") != request.headers.end()) {
			env["CONTENT_LENGTH"] = request.headers.at("Content-Length");
		}
		if (request.headers.find("Content-Type") != request.headers.end()) {
			env["CONTENT_TYPE"] = request.headers.at("Content-Type");
		}
	}

	return env;
}

void CgiHandler::exportEnvironment(const std::map<string, string>& env) {
	for (std::map<string, string>::const_iterator it = env.begin(); 
		 it != env.end(); ++it) {
		if (it->first.empty() || it->first.find('=' != string::npos))
			continue;
		setenv(it->first.c_str(), it->second.c_str(), 1);
	}
}

bool CgiHandler::processCGIResponse(const string& response, int clientSocket) {
	size_t headerEnd = response.find("\r\n\r\n");
	if (headerEnd == string::npos) {
		headerEnd = response.find("\n\n");
		if (headerEnd == string::npos) {
			return false;
		}
	}

	string headers = response.substr(0, headerEnd);
	string body = response.substr(headerEnd + (response[headerEnd + 1] == '\n' ? 2 : 4));

	// Basic header validation
	if (!validateHeaders(headers)) {
		return false;
	}

	std::ostringstream fullResponse;
	fullResponse << "HTTP/1.1 200 OK\r\n";
	
	if (headers.find("Content-Length:") == string::npos) {
		fullResponse << "Content-Length: " << body.length() << "\r\n";
	}
	
	if (headers.find("Content-Type:") == string::npos) {
		fullResponse << "Content-Type: text/html\r\n";
	}

	fullResponse << headers << "\r\n\r\n" << body;
	send(clientSocket, fullResponse.str().c_str(), fullResponse.str().length(), 0);
	return true;
}

bool CgiHandler::validateHeaders(const string& headers) {
	std::istringstream iss(headers);
	string line;
	while (std::getline(iss, line)) {
		if (line.empty() || line == "\r") continue;
		
		// Basic header format validation
		if (line.find(':') == string::npos) return false;
		
		// Check for potentially dangerous headers
		if (line.find("Status:") == 0) {
			string status = line.substr(7);
			int statusCode = std::atoi(status.c_str());
			if (statusCode < 100 || statusCode > 599) return false;
		}
	}
	return true;
}


void CgiHandler::execute(int clientSocket, const HttpServer::HttpRequest& request,
						const LocationCtx& location) {

	std::map<string, string> env = setupEnvironment(request, location);
	string scriptPath = env["SCRIPT_FILENAME"];
	string rootPath = getFirstDirective(location.second, "root")[0];

	// Validate script permissions
	if (access(scriptPath.c_str(), X_OK) != 0) {
		_server.sendError(clientSocket, 403, &location);
		return;
	}

	int pipeFd[2];
	if (pipe(pipeFd) < 0) {
		throw std::runtime_error("Failed to create pipe for CGI");
	}

	pid_t pid = fork();
	if (pid < 0) {
		close(pipeFd[0]);
		close(pipeFd[1]);
		throw std::runtime_error("Fork failed for CGI execution");
	}

	if (pid == 0) {

		close(pipeFd[0]);
		dup2(pipeFd[1], STDOUT_FILENO);
		close(pipeFd[1]);

		if (chdir(rootPath.c_str()) < 0) {
			exit(1);
		}

		// Set resource limits
		struct rlimit rlim;
		rlim.rlim_cur = 30; // 30 seconds CPU time
		rlim.rlim_max = 30;
		setrlimit(RLIMIT_CPU, &rlim);

		rlim.rlim_cur = 100 * 1024 * 1024; // 100MB memory limit
		rlim.rlim_max = 100 * 1024 * 1024;
		setrlimit(RLIMIT_AS, &rlim);
		
		exportEnvironment(env);

		// for some reason if relative specified before it doesn't work at all
		if (Utils::isPrefix(rootPath + "/", scriptPath)) {
			scriptPath = scriptPath.substr(rootPath.length() + 1);
		}

		char *args[] = {
			const_cast<char*>(_program.c_str()),
			const_cast<char*>(scriptPath.c_str()),
			NULL
		};
		execv(args[0], args);
		exit(1);
	}

	close(pipeFd[1]);

	char buffer[4096];
	string response;
	ssize_t bytesRead;
	unsigned long totalSize = 0;
	const size_t maxSize = 10 * 1024 * 1024; // 10MB output limit
	
	while ((bytesRead = read(pipeFd[0], buffer, sizeof(buffer) - 1)) > 0) {
		totalSize += static_cast<unsigned long>(bytesRead); 
		if (totalSize > maxSize) {
			kill(pid, SIGTERM);
			close(pipeFd[0]);
			_server.sendError(clientSocket, 500, &location);
			return;
		}
		buffer[bytesRead] = '\0';
		response += buffer;
	}
	
	close(pipeFd[0]);

	int status;
	if (waitpid(pid, &status, 0) == -1 || WIFSIGNALED(status) || WEXITSTATUS(status) != 0) {
		_server.sendError(clientSocket, 500, &location);
		return;
	}
	// Process CGI output and validate headers
	if (!processCGIResponse(response, clientSocket)) {
		_server.sendError(clientSocket, 502, &location);
		return;
	}
}