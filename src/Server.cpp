// <GENERATED>
#include <iostream> /* std::cout, std::swap, std::ostream */
#include <string> /* std::string */
#include <sstream> /* std::stringstream */

#include "repr.hpp"
#include "Server.hpp"
#include "Logger.hpp"

using std::cout;
using std::swap;
using std::string;
using std::ostream;
using std::stringstream;

// De- & Constructors
Server::~Server() {
	if (Logger::trace())
		cout << ANSI_PUNCT "~" << *this << '\n';
}

Server::Server() : exitStatus(), _config(), _id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "Server" ANSI_PUNCT "() -> " << *this << '\n';
}

Server::Server(unsigned int newExitStatus, const Config& config) : exitStatus(newExitStatus), _config(config), _id(_idCntr++) {
	if (Logger::trace())
		cout << *this << ANSI_PUNCT " -> " << *this << '\n';
}

Server::Server(const Server& other) : exitStatus(other.exitStatus), _config(other._config), _id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "Server" ANSI_PUNCT "(" << ::repr(other) << ANSI_PUNCT ") -> " << *this << '\n';
}

// Copy-assignment operator (using copy-swap idiom)
Server& Server::operator=(Server other) /* noexcept */ {
	if (Logger::trace())
		cout << ANSI_KWRD "Server" ANSI_PUNCT "& " ANSI_KWRD "Server" ANSI_PUNCT "::" ANSI_FUNC "operator" ANSI_PUNCT "=(" << ::repr(other) << ANSI_PUNCT ")" ANSI_RST "\n";
	::swap(*this, other);
	return *this;
}

// Generated getters
int Server::getExitStatus() const { return exitStatus; }
const Config& Server::get_config() const { return _config; }

// Generated setters
void Server::setExitStatus(int value) { exitStatus = value; }
void Server::set_config(const Config& value) { _config = value; }

// Generated member functions
string Server::repr() const {
	stringstream out;
	out << ANSI_KWRD "Server" ANSI_PUNCT "(" << ::repr(exitStatus) << ANSI_PUNCT ", " << ::repr(_config) << ANSI_PUNCT ", " << ::repr(_id) << ANSI_PUNCT ")" ANSI_RST;
	return out.str();
}

void Server::swap(Server& other) /* noexcept */ {
	if (Logger::trace()) {
		cout << ANSI_CMT "<Swapping *this:" ANSI_RST "\n";
		cout << *this << '\n';
		cout << ANSI_CMT "with the following object:" ANSI_RST "\n";
		cout << other << '\n';
	}
	::swap(exitStatus, other.exitStatus);
	::swap(_config, other._config);
	::swap(_id, other._id);
	if (Logger::trace())
		cout << ANSI_CMT "swap done>" ANSI_RST "\n";
}

Server::operator string() const { return ::repr(*this); }

// Generated free functions
void swap(Server& a, Server& b) /* noexcept */ { a.swap(b); }
ostream& operator<<(ostream& os, const Server& other) { return os << static_cast<string>(other); }

// Keeping track of the instances
unsigned int Server::_idCntr = 0;
// </GENERATED>

void Server::serve() {
	cout << "SERVING\n";
}
