// <GENERATED>
#include <iostream> /* std::cout, std::swap, std::ostream */
#include <string> /* std::string */
#include <sstream> /* std::stringstream */
#include <map> /* std::map */

#include "repr.hpp"
#include "ServerConfig.hpp"
#include "CgiHandler.hpp"
#include "Logger.hpp"

using std::cout;
using std::map;
using std::swap;
using std::string;
using std::ostream;
using std::stringstream;

// De- & Constructors
ServerConfig::~ServerConfig() {
	if (Logger::trace())
		cout << ANSI_PUNCT "~" << *this << '\n';
}

ServerConfig::ServerConfig() :
		BaseConfig(),
		_serverName(),
		_listenAddress(),
		_listenPort(),
		_id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "ServerConfig" ANSI_PUNCT "() -> " << *this << '\n';
}

ServerConfig::ServerConfig(
	const string& accessLog,
	const string& errorLog,
	unsigned int maxBodySize,
	const string& root,
	const string& errorPage,
	bool enableDirListing,
	const vector<string>& indexFiles,
	const map<string, CgiHandler>& cgiHandlers,
	const string& serverName,
	const string& listenAddress,
	unsigned int listenPort) :
		BaseConfig(accessLog, errorLog, maxBodySize, root, errorPage, enableDirListing, indexFiles, cgiHandlers),
		_serverName(serverName),
		_listenAddress(listenAddress),
		_listenPort(listenPort),
		_id(_idCntr++) {
	if (Logger::trace())
		cout << *this << ANSI_PUNCT " -> " << *this << '\n';
}

ServerConfig::ServerConfig(const ServerConfig& other) :
		BaseConfig(other),
		_serverName(other._serverName),
		_listenAddress(other._listenAddress),
		_listenPort(other._listenPort),
		_id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "ServerConfig" ANSI_PUNCT "(" << ::repr(other) << ANSI_PUNCT ") -> " << *this << '\n';
}

// Copy-assignment operator (using copy-swap idiom)
ServerConfig& ServerConfig::operator=(ServerConfig other) /* noexcept */ {
	if (Logger::trace())
		cout << ANSI_KWRD "ServerConfig" ANSI_PUNCT "& " ANSI_KWRD "ServerConfig" ANSI_PUNCT "::" ANSI_FUNC "operator" ANSI_PUNCT "=(" << ::repr(other) << ANSI_PUNCT ")" ANSI_RST "\n";
	::swap(*this, other);
	return *this;
}

const string& ServerConfig::getServerName() const { return _serverName; }
const string& ServerConfig::getListenAddress() const { return _listenAddress; }
unsigned int ServerConfig::getListenPort() const { return _listenPort; }

void ServerConfig::setServerName(const string& value) { _serverName = value; }
void ServerConfig::setListenAddress(const string& value) { _listenAddress = value; }
void ServerConfig::setListenPort(unsigned int value) { _listenPort = value; }

// Generated member functions
string ServerConfig::repr() const {
	stringstream out;
	out << ANSI_KWRD "ServerConfig" ANSI_PUNCT "("
		<< ::repr(_accessLog) << ANSI_PUNCT ", "
		<< ::repr(_errorLog) << ANSI_PUNCT ", "
		<< ::repr(_maxBodySize) << ANSI_PUNCT ", "
		<< ::repr(_enableDirListing) << ANSI_PUNCT ", "
		<< ::repr(_indexFiles) << ANSI_PUNCT ", "
		<< ::repr(_cgiHandlers) << ANSI_PUNCT ", "

		<< ::repr(_serverName) << ANSI_PUNCT ", "
		<< ::repr(_listenAddress) << ANSI_PUNCT ", "
		<< ::repr(_listenPort) << ANSI_PUNCT ", "
		<< ::repr(_id) << ANSI_PUNCT ")" ANSI_RST;
	return out.str();
}

void ServerConfig::swap(ServerConfig& other) /* noexcept */ {
	if (Logger::trace()) {
		cout << ANSI_CMT "<Swapping ServerConfig *this:" ANSI_RST "\n";
		cout << *this << '\n';
		cout << ANSI_CMT "with the following ServerConfig object:" ANSI_RST "\n";
		cout << other << '\n';
	}
	::swap(_accessLog, other._accessLog);
	::swap(_errorLog, other._errorLog);
	::swap(_maxBodySize, other._maxBodySize);
	::swap(_enableDirListing, other._enableDirListing);
	::swap(_indexFiles, other._indexFiles);
	::swap(_cgiHandlers, other._cgiHandlers);

	::swap(_serverName, other._serverName);
	::swap(_listenAddress, other._listenAddress);
	::swap(_listenPort, other._listenPort);
	::swap(_id, other._id);
	if (Logger::trace())
		cout << ANSI_CMT "ServerConfig swap done>" ANSI_RST "\n";
}

ServerConfig::operator string() const { return ::repr(*this); }

// Generated free functions
void swap(ServerConfig& a, ServerConfig& b) /* noexcept */ { a.swap(b); }
ostream& operator<<(ostream& os, const ServerConfig& other) { return os << static_cast<string>(other); }

// Keeping track of the instances
unsigned int ServerConfig::_idCntr = 0;
// </GENERATED>
