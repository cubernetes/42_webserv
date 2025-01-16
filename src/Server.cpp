#include <iostream> /* std::cout, std::swap, std::ostream */
#include <string> /* std::string */
#include <sstream> /* std::stringstream */

// #include "repr.hpp"
#include "Server.hpp"
#include "Logger.hpp"
#include "Constants.hpp"
#include "conf.hpp"

using std::cout;
using std::swap;
using std::string;
using std::ostream;
using std::stringstream;

// De- & Constructors
Server::~Server() {
	// TRACE_DTOR;
}

Server::Server() : exitStatus(), rawConfig(readConfig(Constants::defaultConfPath)), config(parseConfig(rawConfig)), _http(), _id(_idCntr++) {
	// reflect(); TRACE_DEFAULT_CTOR;
}

Server::Server(const string& confPath) : exitStatus(), rawConfig(readConfig(confPath)), config(parseConfig(rawConfig)), _http(), _id(_idCntr++) {
	// reflect();
	// if (Logger::trace()) { // TODO: abstract away
	// 	if (Constants::jsonTrace)
	// 		cout << "{\"event\":\"string confPath constructor\",\"this object\":" << ::repr(*this) << "}\n";
	// 	else
	// 		cout << kwrd(_class) + punct("(") << ::repr(confPath) << punct(") -> ") << ::repr(*this) << '\n';
	// }
}

Server::Server(const Server& other) : exitStatus(other.exitStatus), rawConfig(other.rawConfig), config(other.config), _http(other._http), _id(_idCntr++) {
	// reflect(); TRACE_COPY_CTOR;
}

// Copy-assignment operator (using copy-swap idiom)
Server& Server::operator=(Server other) /* noexcept */ {
	// TRACE_COPY_ASSIGN_OP;
	::swap(*this, other);
	return *this;
}

void Server::swap(Server& other) /* noexcept */ {
	// TRACE_SWAP_BEGIN;
	::swap(exitStatus, other.exitStatus);
	::swap(config, other.config);
	::swap(_id, other._id);
	// TRACE_SWAP_END;
}

Server::operator string() const { return ::repr(*this); }

void swap(Server& a, Server& b) /* noexcept */ { a.swap(b); }
ostream& operator<<(ostream& os, const Server& other) { return os << static_cast<string>(other); }

// Keeping track of the instances
unsigned int Server::_idCntr = 0;

void Server::serve() {
	cout << "SERVING...\n";
}
