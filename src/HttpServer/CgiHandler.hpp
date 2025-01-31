#pragma once /* CgiHandler.hpp */

#include "HttpServer.hpp"

using std::string;

class CgiHandler {
public:
  ~CgiHandler();
  CgiHandler(HttpServer &server, const string &extension, const string &program, Logger &_log);
  CgiHandler(const CgiHandler &other);
  CgiHandler &operator=(CgiHandler);
  void swap(CgiHandler &); // copy swap idiom

  // string conversion
  operator string() const;

  const string &get_extension() const;
  const string &get_program() const;

  bool canHandle(const string &path) const;
  void execute(int clientSocket, const HttpServer::HttpRequest &request, const LocationCtx &location);
  static bool validateHeaders(const string &headers);

private:
  HttpServer &_server;
  string _extension;
  string _program;
  Logger &log;

  char **exportEnvironment(const std::map<string, string> &env, size_t &n);
  std::map<string, string> setupEnvironment(const HttpServer::HttpRequest &request, const LocationCtx &location);
};

void swap(CgiHandler &, CgiHandler &) /* noexcept */;
std::ostream &operator<<(std::ostream &, const CgiHandler &);
