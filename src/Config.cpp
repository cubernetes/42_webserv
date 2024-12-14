// <GENERATED>
#include <fstream> /* std::ifstream */
#include <iostream> /* std::cout, std::swap, std::ostream */
#include <stdexcept>
#include <string> /* std::string */
#include <sstream> /* std::stringstream */
#include <map> /* std::map */

#include "repr.hpp"
#include "BaseConfig.hpp"
#include "Config.hpp"
#include "ServerConfig.hpp"
#include "CgiHandler.hpp"
#include "Errors.hpp"
#include "Logger.hpp"

using std::cout;
using std::map;
using std::ifstream;
using std::swap;
using std::string;
using std::ostream;
using std::stringstream;
using std::runtime_error;

// De- & Constructors
Config::~Config() {
	if (Logger::trace())
		cout << ANSI_PUNCT "~" << *this << '\n';
}

Config::Config() :
		BaseConfig(),
		_configPath(),
		_workerConfigs(),
		_maxWorkers(),
		_id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "Config" ANSI_PUNCT "() -> " << *this << '\n';
}

Config::Config(const string& configPath) :
		BaseConfig(),
		_configPath(configPath),
		_workerConfigs(),
		_maxWorkers(),
		_id(_idCntr++) {
	parse();
	if (Logger::trace())
		cout << ANSI_KWRD "Config" ANSI_PUNCT "(" << ::repr(configPath) << ANSI_PUNCT ") -> " << *this << '\n';
}

Config::Config(
	const string& accessLog,
	const string& errorLog,
	unsigned int maxBodySize,
	const string& root,
	const string& errorPage,
	bool enableDirListing,
	const vector<string>& indexFiles,
	const map<string, CgiHandler>& cgiHandlers,
	const string& configPath,
	const vector<ServerConfig>& serverConfigs,
	unsigned int maxWorkers
	) :
		BaseConfig(accessLog, errorLog, maxBodySize, root, errorPage, enableDirListing, indexFiles, cgiHandlers),
		_configPath(configPath),
		_workerConfigs(serverConfigs),
		_maxWorkers(maxWorkers),
		_id(_idCntr++) {
	if (Logger::trace())
		cout << *this << ANSI_PUNCT " -> " << *this << '\n';
}

Config::Config(const Config& other) :
		BaseConfig(other),
		_configPath(other._configPath),
		_workerConfigs(other._workerConfigs),
		_maxWorkers(other._maxWorkers),
		_id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "Config" ANSI_PUNCT "(" << ::repr(other) << ANSI_PUNCT ") -> " << *this << '\n';
}

// Copy-assignment operator (using copy-swap idiom)
Config& Config::operator=(Config other) /* noexcept */ {
	if (Logger::trace())
		cout << ANSI_KWRD "Config" ANSI_PUNCT "& " ANSI_KWRD "Config" ANSI_PUNCT "::" ANSI_FUNC "operator" ANSI_PUNCT "=(" << ::repr(other) << ANSI_PUNCT ")" ANSI_RST "\n";
	::swap(*this, other);
	return *this;
}

// Generated getters
const string& Config::getConfigPath() const { return _configPath; }
const vector<ServerConfig>& Config::getServerConfigs() const { return _workerConfigs; }
unsigned int Config::getMaxWorkers() const { return _maxWorkers; }

// Generated setters
void Config::setConfigPath(const string& value) { _configPath = value; }
void Config::setServerConfigs(const vector<ServerConfig>& value) { _workerConfigs = value; }
void Config::setMaxWorkers(unsigned int value) { _maxWorkers = value; }

// Generated member functions
string Config::repr() const {
	stringstream out;
	out << ANSI_KWRD "Config" ANSI_PUNCT "("
		<< ::repr(_accessLog) << ANSI_PUNCT ", "
		<< ::repr(_errorLog) << ANSI_PUNCT ", "
		<< ::repr(_maxBodySize) << ANSI_PUNCT ", "
		<< ::repr(_root) << ANSI_PUNCT ", "
		<< ::repr(_errorPage) << ANSI_PUNCT ", "
		<< ::repr(_enableDirListing) << ANSI_PUNCT ", "
		<< ::repr(_indexFiles) << ANSI_PUNCT ", "
		<< ::repr(_cgiHandlers) << ANSI_PUNCT ", "

		<< ::repr(_configPath) << ANSI_PUNCT ", "
		<< ::repr(_workerConfigs) << ANSI_PUNCT ", "
		<< ::repr(_maxWorkers) << ANSI_PUNCT ", "

		<< ::repr(_id) << ANSI_PUNCT ")" ANSI_RST;
	return out.str();
}

void Config::swap(Config& other) /* noexcept */ {
	if (Logger::trace()) {
		cout << ANSI_CMT "<Swapping Config *this:" ANSI_RST "\n";
		cout << *this << '\n';
		cout << ANSI_CMT "with the following Config object:" ANSI_RST "\n";
		cout << other << '\n';
	}
	::swap(*dynamic_cast<BaseConfig*>(this), *dynamic_cast<BaseConfig*>(&other)),
	::swap(_configPath, other._configPath);
	::swap(_workerConfigs, other._workerConfigs);
	::swap(_maxWorkers, other._maxWorkers);
	::swap(_id, other._id);
	if (Logger::trace())
		cout << ANSI_CMT "Config swap done>" ANSI_RST "\n";
}

Config::operator string() const { return ::repr(*this); }

// Generated free functions
void swap(Config& a, Config& b) /* noexcept */ { a.swap(b); }
ostream& operator<<(ostream& os, const Config& other) { return os << static_cast<string>(other); }

// Keeping track of the instances
unsigned int Config::_idCntr = 0;
// </GENERATED>

void Config::parse() {
	std::ifstream f(_configPath.c_str());
	if (!f.good())
		throw runtime_error(Errors::ErrorOpeningConfig);
	_accessLog = "log/server1-acc.log";
	_errorLog = "log/server1-err.log";
	_maxBodySize = 4096;
	_root = "www";
	_errorPage = "www/404.html";
	_enableDirListing = true;
	_indexFiles.push_back("index.html");
	_indexFiles.push_back("index.htm");
	_indexFiles.push_back("webserv-index.html");
	_cgiHandlers["py"] = CgiHandler("py");
	_cgiHandlers["php"] = CgiHandler("php");
	_workerConfigs.push_back(ServerConfig(this->_accessLog, this->_errorLog, this->_maxBodySize, this->_root, this->_errorPage, this->_enableDirListing, this->_indexFiles, this->_cgiHandlers, "Server1", "127.0.0.1", 8080));
	_maxWorkers = 8;
}
