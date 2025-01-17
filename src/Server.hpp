#pragma once /* Server.hpp */

#include <string>
#include <iostream>

#include "HttpServer.hpp"
#include "conf.hpp"

using std::string;
using std::ostream;

class Server {
public:
	~Server();
	Server();
	Server(const string& confPath);
	Server(const Server& other);
	Server& operator=(Server);
	void swap(Server& other);
	operator string() const;

	void serve();

	unsigned int get_exitStatus() const;
	const string& get_rawConfig() const;
	const t_config& get_config() const;
	const HttpServer& get_http() const;
private:
	unsigned int _exitStatus;
	string _rawConfig;
	t_config _config;
	HttpServer _http;
};

void swap(Server&, Server&) /* noexcept */;
ostream& operator<<(ostream&, const Server&);
