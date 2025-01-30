#include "CgiHandler.hpp"
#include "ReprCgi.hpp"
#include <cstdio>
#include <cstring>

#define PIPE_READ 0
#define PIPE_WRITE 1

using std::swap;
using std::string;
typedef HttpServer::ClientFdToCgiMap CgiProcessMap; // not sure why using doesn't work with this one

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
	env["SCRIPT_FILENAME"] = rootPath + relativePath; // TODO: @all: it should be with cgi_dir directive
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

char **CgiHandler::exportEnvironment(const std::map<string, string>& env) {
	size_t size = env.size();
	char **envArr = new char*[size + 1];
	size_t i = 0;
	for (std::map<string, string>::const_iterator it = env.begin(); it != env.end(); ++it) {
		if (it->first.empty() || it->first.find('=') != string::npos)
			continue;
		envArr[i] = ::strdup((it->first + "=" + it->second).c_str());
		++i;
	}
	envArr[i] = NULL;
	return envArr;
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

	int toCgi[2];
	int fromCgi[2];
	if (pipe(toCgi) < 0)
		throw std::runtime_error("Failed to create input pipe for CGI");
	if (pipe(fromCgi) < 0)
		throw std::runtime_error("Failed to create output pipe for CGI");

	// (deprecated) Make both ends non-blocking
	// fcntl(toCgi[PIPE_READ], F_SETFL, O_NONBLOCK);
	// fcntl(toCgi[PIPE_WRITE], F_SETFL, O_NONBLOCK);
	// fcntl(fromCgi[PIPE_READ], F_SETFL, O_NONBLOCK);
	// fcntl(fromCgi[PIPE_WRITE], F_SETFL, O_NONBLOCK);
	
	pid_t pid = fork();
	if (pid < 0) {
		close(toCgi[PIPE_READ]);
		close(toCgi[PIPE_WRITE]);
		close(fromCgi[PIPE_READ]);
		close(fromCgi[PIPE_WRITE]);
		throw std::runtime_error("Fork failed for CGI execution");
	}

	if (pid == 0) { // we are in child
		close(toCgi[PIPE_WRITE]); // doesn't need to write to itself
		close(fromCgi[PIPE_READ]); // doesn't need to read from itself

		dup2(toCgi[PIPE_READ], STDIN_FILENO); // whenever something writes to toCgi WRITE end, then CGI will receive it via stdin
		dup2(fromCgi[PIPE_WRITE], STDOUT_FILENO); // whenever CGI writes something to stdout, it will go to WRITE end of formCgi pipe

		close(toCgi[PIPE_READ]);
		close(fromCgi[PIPE_WRITE]);

		if (chdir(rootPath.c_str()) < 0) { // TODO: @all: use cgi_dir as well
			exit(1);
		}
		
		char **cgiEnviron = exportEnvironment(env); // allocates on the heap!

		// for some reason if relative specified before it doesn't work at all
		if (Utils::isPrefix(rootPath + "/", scriptPath)) { // TODO: @all: use cgi_dir as well
			scriptPath = scriptPath.substr(rootPath.length() + 1);
		}

		char *args[] = {
			const_cast<char*>(_program.c_str()),
			const_cast<char*>(scriptPath.c_str()),
			NULL
		};
		execve(args[0], args, cgiEnviron);
		exit(1);
	}

	close(toCgi[PIPE_READ]); // parent process should only write to cgi process
	close(fromCgi[PIPE_WRITE]); // parent process should only read from cgi process
	int cgiWriteFd = toCgi[PIPE_WRITE];
	int cgiReadFd = fromCgi[PIPE_READ];

	std::pair<CgiProcessMap::iterator, bool> result;
	result = _server._clientToCgi.insert(std::make_pair(clientSocket, HttpServer::CgiProcess(pid, cgiReadFd, cgiWriteFd, clientSocket, &location)));
	(void)result;

	// from timo: commented out for now
	// if (!result.second) {
	// 	// If key already exists, update the existing entry
	// 	// TODO: @discuss: is this even correct? what happens to the old CgiProcess entry?
	// 	result.first->second = HttpServer::CgiProcess(pid, cgiReadFd, clientSocket, &location);
	// }

	struct pollfd pfd;
	pfd.fd = cgiReadFd;
	pfd.events = POLLIN; // get notified when CGI has data ready to send
	_server._cgiToClient[cgiReadFd] = clientSocket;
	_server._monitorFds.pollFds.push_back(pfd);

	struct pollfd pfd2;
	pfd2.fd = cgiWriteFd;
	pfd2.events = POLLOUT; // for the pendingWrite
	_server._cgiToClient[cgiWriteFd] = clientSocket;
	_server._monitorFds.pollFds.push_back(pfd2);

	_server.queueWrite(cgiWriteFd, request.body);
}
