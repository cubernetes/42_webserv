#pragma once /* Server.hpp */

#include <string>
#include <iostream>

#include "HttpServer.hpp"
#include "Config.hpp"

using std::string;
using std::ostream;

class Server {
public:
	~Server();
	Server();
	Server(const string& confPath);
	Server(const Server& other);
	Server& operator=(Server);
	void swap(Server& other); // copy swap idiom

	// string conversion
	operator string() const;

	void serve();

	// Getters
	unsigned int get_exitStatus() const;
	const string& get_rawConfig() const;
	const Config& get_config() const;
	const HttpServer& get_http() const;
	unsigned int get_id() const;
private:
	unsigned int _exitStatus;
	string _rawConfig;
	Config _config;
	HttpServer _http;

	// Instance tracking
	unsigned int _id;
	static unsigned int _idCntr;
};

// global scope swap (aka ::swap), needed since friend keyword is forbidden :(
void swap(Server&, Server&) /* noexcept */;
ostream& operator<<(ostream&, const Server&);
