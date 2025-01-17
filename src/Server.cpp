#include <iostream>
#include <ostream>
#include <string>

#include "Server.hpp"
#include "Logger.hpp"
#include "Constants.hpp"
#include "Config.hpp"

using std::cout;
using std::swap;
using std::string;

// De- & Constructors
Server::~Server() {
	TRACE_DTOR;
}

Server::Server() :
	_exitStatus(),
	_rawConfig(readConfig(Constants::defaultConfPath)),
	_config(parseConfig(_rawConfig)),
	_http(),
	_id(_idCntr++) {
	TRACE_DEFAULT_CTOR;
}

Server::Server(const string& confPath) :
	_exitStatus(),
	_rawConfig(readConfig(confPath)),
	_config(parseConfig(_rawConfig)),
	_http(),
	_id(_idCntr++) {
	if (Logger::trace()) { // TODO: @timo: abstract away
		if (Constants::jsonTrace)
			cout << "{\"event\":\"string confPath constructor\",\"this object\":" << ::repr(*this) << "}\n";
		else
			cout << kwrd(getClass(*this)) + punct("(") << ::repr(confPath) << punct(") -> ") << ::repr(*this) << '\n';
	}
}

Server::Server(const Server& other) :
	_exitStatus(other._exitStatus),
	_rawConfig(other._rawConfig),
	_config(other._config),
	_http(other._http),
	_id(_idCntr++) {
	TRACE_COPY_CTOR;
}

// Instance tracking
unsigned int Server::_idCntr = 0;

// copy swap idiom
Server& Server::operator=(Server other) /* noexcept */ {
	TRACE_COPY_ASSIGN_OP;
	::swap(*this, other);
	return *this;
}

// Getters
unsigned int Server::get_exitStatus() const { return _exitStatus; }
const string& Server::get_rawConfig() const { return _rawConfig; }
const Config& Server::get_config() const { return _config; }
const HttpServer& Server::get_http() const { return _http; }
unsigned int Server::get_id() const { return _id; }

void Server::swap(Server& other) /* noexcept */ {
	TRACE_SWAP_BEGIN;
	::swap(_exitStatus, other._exitStatus);
	::swap(_rawConfig, other._rawConfig);
	::swap(_config, other._config);
	::swap(_http, other._http);
	::swap(_id, other._id);
	TRACE_SWAP_END;
}

Server::operator string() const { return ::repr(*this); }

void swap(Server& a, Server& b) /* noexcept */ {
	a.swap(b);
}

std::ostream& operator<<(std::ostream& os, const Server& other) {
	return os << static_cast<string>(other);
}
// end of boilerplate


void Server::serve() {
	cout << "SERVING...\n";
}
