#include "CgiHandler.hpp"
#include "ReprCgi.hpp"

using std::swap;
using std::string;

extern char **environ;

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
	env["PATH_INFO"] = (scriptEnd != string::npos && scriptEnd + 3 < request.path.length()) 
		? request.path.substr(scriptEnd + 3) 
		: "/";
	
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
	// TODO: @sonia: eval sheet says we HAVE to check send for < 0 and == 0, not sure what to do here :/
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

	int sockets[2];
	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, sockets) < 0) {
		throw std::runtime_error("Failed to create socket pair for CGI");
	}

	 // Make both ends non-blocking
	fcntl(sockets[0], F_SETFL, O_NONBLOCK);
	fcntl(sockets[1], F_SETFL, O_NONBLOCK);
	
	pid_t pid = fork();
	if (pid < 0) {
		close(sockets[0]);
		close(sockets[1]);
		throw std::runtime_error("Fork failed for CGI execution");
	}

	if (pid == 0) {

		close(sockets[0]);
		dup2(sockets[1], STDOUT_FILENO);
		close(sockets[1]);

		if (chdir(rootPath.c_str()) < 0) {
			exit(1);
		}
		
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
		execve(args[0], args, environ);
		exit(1);
	}

	close(sockets[1]);

	std::map<int, HttpServer::CGIProcess>& cgiProcesses = _server.get_CGIProcesses();
	std::pair<std::map<int, HttpServer::CGIProcess>::iterator, bool> result;
	result = cgiProcesses.insert(std::make_pair(sockets[0], HttpServer::CGIProcess(pid, sockets[0], clientSocket, &location)));

	if (!result.second) {
		// If key already exists, update the existing entry
		result.first->second = HttpServer::CGIProcess(pid, sockets[0], clientSocket, &location);
	}

	struct pollfd pfd;
	pfd.fd = sockets[0];
	pfd.events = POLLIN;
	_server.get_MonitorFds().pollFds.push_back(pfd);
}
