#include <algorithm>
#include <cctype>
#include <cerrno>

#include <cstdlib>
#include <cstring>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <sys/poll.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include "CgiHandler.hpp"
#include "Config.hpp"
#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

#define PIPE_READ 0
#define PIPE_WRITE 1

using std::string;
using std::swap;
typedef HttpServer::ClientFdToCgiMap CgiProcessMap; // not sure why using doesn't work with this one

// De- & Constructors
CgiHandler::~CgiHandler() { TRACE_DTOR; }

CgiHandler::CgiHandler(HttpServer &server, const string &extension, const string &program, Logger &_log)
    : _server(server), _extension(extension), _program(program), log(_log) {
    TRACE_ARG_CTOR(string, extension, string, program);
}

CgiHandler::CgiHandler(const CgiHandler &other)
    : _server(other._server), _extension(other._extension), _program(other._program), log(other.log) {
    TRACE_COPY_CTOR;
}

// copy swap idiom
CgiHandler &CgiHandler::operator=(CgiHandler other) /* noexcept */ {
    TRACE_COPY_ASSIGN_OP;
    ::swap(*this, other);
    return *this;
}

// Getters
const string &CgiHandler::get_extension() const { return _extension; }
const string &CgiHandler::get_program() const { return _program; }

void CgiHandler::swap(CgiHandler &other) /* noexcept */ {
    TRACE_SWAP_BEGIN;
    ::swap(_extension, other._extension);
    ::swap(_program, other._program);
    ::swap(log, other.log);
    TRACE_SWAP_END;
}

CgiHandler::operator string() const { return ::repr(*this); }

void swap(CgiHandler &a, CgiHandler &b) /* noexcept */ { a.swap(b); }

std::ostream &operator<<(std::ostream &os, const CgiHandler &other) { return os << static_cast<string>(other); }

bool CgiHandler::canHandle(const string &path) const {
    return path.length() > _extension.length() && path.substr(path.length() - _extension.length()) == _extension;
}

string getHost(const HttpServer::HttpRequest &request);

std::map<string, string> CgiHandler::setupEnvironment(const HttpServer::HttpRequest &request,
                                                      const LocationCtx &location) {
    std::map<string, string> env;

    string rootPath = getFirstDirective(location.second, "root")[0];
    string relativePath = request.path;
    string extension = request.path.substr(request.path.rfind("."));
    if (Utils::isPrefix(rootPath, request.path)) {
        relativePath = request.path.substr(rootPath.length());
    }

    // Find any path info after the script name
    size_t scriptEnd = request.path.find(extension);
    env["PATH_INFO"] = (scriptEnd != string::npos && scriptEnd + extension.length() < request.path.length())
                           ? request.path.substr(scriptEnd + extension.length())
                           : "/";

    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["SERVER_SOFTWARE"] = "webserv/1.0";
    env["REQUEST_METHOD"] = request.method;
    env["SCRIPT_FILENAME"] = rootPath + relativePath;
    env["SCRIPT_NAME"] = relativePath;
    env["SERVER_NAME"] = getHost(request);
    // env["REMOTE_ADDR"] = getHost(request); // TODO: @all: maybe a different time
    env["QUERY_STRING"] = "";

    size_t queryPos = request.path.find('?');
    if (queryPos != string::npos) {
        env["QUERY_STRING"] = request.path.substr(queryPos + 1);
    }

    for (std::map<string, string>::const_iterator it = request.headers.begin(); it != request.headers.end(); ++it) {
        string headerName = "HTTP_" + it->first;
        std::transform(headerName.begin(), headerName.end(), headerName.begin(),
                       static_cast<int (*)(int)>(std::toupper));
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

static size_t ft_strlen(char const *s) {
    size_t length;

    length = 0;
    while (*s++)
        length++;
    return (length);
}

static size_t ft_strlcpy(char *dst, char const *src, size_t size) {
    size_t len;

    len = 0;
    while (len + 1 < size && src[len])
        *dst++ = src[len++];
    if (size)
        *dst = 0;
    while (src[len])
        len++;
    return (len);
}

static char *ft_strdup(char const *s) {
    char *s2;
    size_t length;

    length = ft_strlen(s);
    s2 = new char[length + 1];
    if (!s2)
        return (0);
    ft_strlcpy(s2, s, length + 1);
    return (s2);
}

char **CgiHandler::exportEnvironment(const std::map<string, string> &env, size_t &n) {
    size_t size = env.size();
    char **envArr = new char *[size + 1];
    size_t i = 0;
    for (std::map<string, string>::const_iterator it = env.begin(); it != env.end(); ++it) {
        if (it->first.empty() || it->first.find('=') != string::npos)
            continue;
        envArr[i] = ft_strdup((it->first + "=" + it->second).c_str());
        ++i;
    }
    envArr[i] = NULL;
    n = i;
    return envArr;
}

bool CgiHandler::validateHeaders(const string &headers) {
    std::istringstream iss(headers);
    string line;
    while (std::getline(iss, line)) {
        if (line.empty() || line == "\r")
            continue;

        // Basic header format validation
        if (line.find(':') == string::npos)
            return false;

        // Check for potentially dangerous headers
        if (line.find("Status:") == 0) {
            string status = line.substr(7);
            int statusCode = std::atoi(status.c_str());
            if (statusCode < 100 || statusCode > 599)
                return false;
        }
    }
    return true;
}

void CgiHandler::execute(int clientSocket, const HttpServer::HttpRequest &request, const LocationCtx &location) {

    std::map<string, string> env = setupEnvironment(request, location);
    string scriptPath = env["SCRIPT_FILENAME"];
    string rootPath = getFirstDirective(location.second, "root")[0];
    string cgiDir = getFirstDirective(location.second, "cgi_dir")[0];

    int toCgi[2];
    int fromCgi[2];
    if (pipe(toCgi) < 0)
        throw std::runtime_error("Failed to create input pipe for CGI");
    if (pipe(fromCgi) < 0)
        throw std::runtime_error("Failed to create output pipe for CGI");

    log.debug() << "Calling fork()" << std::endl;
    pid_t pid = fork();
    if (pid < 0) {
        log.debug() << "Error with fork(): " << strerror(errno) << std::endl;
        close(toCgi[PIPE_READ]);
        close(toCgi[PIPE_WRITE]);
        close(fromCgi[PIPE_READ]);
        close(fromCgi[PIPE_WRITE]);
        throw std::runtime_error("Fork failed for CGI execution");
    }

    if (pid == 0) { // we are in child
        log.debug() << "Child: Entered, about to close FDs" << std::endl;
        close(toCgi[PIPE_WRITE]);  // doesn't need to write to itself
        close(fromCgi[PIPE_READ]); // doesn't need to read from itself

        dup2(toCgi[PIPE_READ],
             STDIN_FILENO); // whenever something writes to toCgi WRITE end, then CGI will receive it via stdin
        dup2(fromCgi[PIPE_WRITE],
             STDOUT_FILENO); // whenever CGI writes something to stdout, it will go to WRITE end of formCgi pipe

        close(toCgi[PIPE_READ]);
        close(fromCgi[PIPE_WRITE]);

        log.debug() << "Child: Changing dir to " << rootPath + cgiDir << std::endl;
        if (chdir((rootPath + cgiDir).c_str()) < 0) {
            exit(1);
        }

        // cgiDir is guaranteed to be a prefix, and needs to be removed since we cd'd into it
        /*       /foo  - /foo/test.py -> /test.py -> .//test.py
         *       /foo/ - /foo/test.py ->  test.py ->  ./test.py
         */
        scriptPath = "./" + request.path.substr(cgiDir.length());

        char *a = ::getcwd(NULL, 0);
        log.debug() << "CWD: " << a << std::endl;
        ::free(a);

        size_t n;
        char **cgiEnviron = exportEnvironment(env, n); // allocates on the heap!

        string argv0, argv1;
        if (!_program.empty()) {
            argv0 = _program;
            argv1 = scriptPath;
        } else {
            argv0 = scriptPath;
            argv1 = "";
        }
        char *args[] = {const_cast<char *>(argv0.c_str()), const_cast<char *>(argv1.empty() ? NULL : argv1.c_str()),
                        NULL};
        log.debug() << "Child: Calling execve(" << repr(args[0]) << ", "
                    << reprArr((char **)args, 2, Constants::jsonTrace) << ", "
                    << reprArr(cgiEnviron, n + 1, Constants::jsonTrace) << ")" << std::endl;
        execve(args[0], args, cgiEnviron);
        log.warn() << "Child: execve failed" << std::endl;
        exit(1);
    }

    close(toCgi[PIPE_READ]);    // parent process should only write to cgi process
    close(fromCgi[PIPE_WRITE]); // parent process should only read from cgi process
    int cgiWriteFd = toCgi[PIPE_WRITE];
    int cgiReadFd = fromCgi[PIPE_READ];

    std::pair<CgiProcessMap::iterator, bool> result;
    result = _server._clientToCgi.insert(
        std::make_pair(clientSocket, HttpServer::CgiProcess(pid, cgiReadFd, cgiWriteFd, clientSocket, &location)));
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

    log.debug() << "Parent: Queueing data to write to CGI process (fd=" << cgiWriteFd << "): [" << Utils::escape(cgiDir)
                << "]" << std::endl;
    _server.queueWrite(cgiWriteFd, request.body);
}
