#pragma once /* Server.hpp */

#include <string>
#include <iostream>

#include "Reflection.hpp"
#include "HttpServer.hpp"
#include "conf.hpp"

using std::string;
using std::ostream;

class Server : public Reflection {
public:
	~Server();
	Server();
	Server(const string& confPath);
	Server(const Server& other);
	Server& operator=(Server);
	void swap(Server& other);
	operator string() const;

	void serve();
public:
	REFLECT(
		Server,
		(unsigned int, exitStatus),
		(const string, rawConfig),
		(t_config, config), // maybe private, since non-const?
		(HttpServer, _http), // should be private
		(unsigned int, _id) // should be private
	)
private:
	static unsigned int _idCntr;
};

void swap(Server&, Server&) /* noexcept */;
ostream& operator<<(ostream&, const Server&);
