// <GENERATED>
#include <cctype> /* isspace */
#include <deque> /* std::deque */
#include <fstream> /* std::ifstream */
#include <iostream> /* std::cout, std::swap, std::ostream */
#include <stdexcept> /* std::runtime_error */
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
using std::deque;
using std::vector;

// De- & Constructors
Config::~Config() {
	if (Logger::trace())
		cout << ANSI_PUNCT "~" << *this << '\n';
}

Config::Config() :
		BaseConfig(),
		_configPath(),
		_serverConfigs(),
		_maxWorkers(),
		_id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "Config" ANSI_PUNCT "() -> " << *this << '\n';
}

Config::Config(const string& configPath) :
		BaseConfig(),
		_configPath(configPath),
		_serverConfigs(),
		_maxWorkers(),
		_id(_idCntr++) {
	//parse();
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
		_serverConfigs(serverConfigs),
		_maxWorkers(maxWorkers),
		_id(_idCntr++) {
	if (Logger::trace())
		cout << *this << ANSI_PUNCT " -> " << *this << '\n';
}

Config::Config(const Config& other) :
		BaseConfig(other),
		_configPath(other._configPath),
		_serverConfigs(other._serverConfigs),
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
const vector<ServerConfig>& Config::getServerConfigs() const { return _serverConfigs; }
unsigned int Config::getMaxWorkers() const { return _maxWorkers; }

// Generated setters
void Config::setConfigPath(const string& value) { _configPath = value; }
void Config::setServerConfigs(const vector<ServerConfig>& value) { _serverConfigs = value; }
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
		<< ::repr(_serverConfigs) << ANSI_PUNCT ", "
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
	::swap(_serverConfigs, other._serverConfigs);
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


/*

config ::= directives? http_block
directives ::= directive directives?
directive ::= directive_name arguments ';'
directive_name ::= TOK_WORD
arguments ::= argument arguments?
argument ::= TOK_WORD
http_block ::= '{' server_list '}'
server_list ::= server server_list?
server ::= '{' server_configs '}'
server_configs ::= [ directive | route ] server_configs?
route ::= 'location' route_pattern '{' directives '}'
route_pattern ::= TOK_WORD

*/

bool Config::_accept(t_tokens& tokens, token_type tok_type) {
	if (tokens.empty())
		return false;
	if (tokens.front().first != tok_type)
		return false;
	tokens.pop_front();
	return true;
}

bool Config::_expect(t_tokens& tokens, token_type tok_type) {
	if (_accept(tokens, tok_type))
		return true;
	_unexpected(tokens);
	return false;
}

void Config::_unexpected(t_tokens& tokens) {
	// TODO: use logger
	if (tokens.empty())
		cout << "Unexpected end of file\n";
	else
		cout << "Unexpected token: '" << tokens.front().second << "'\n";
}

t_token Config::_new_token(token_type t, const string& s) {
	return t_token(t, s);
}

// not sure yet, but I guess:
/*
// route_pattern ::= TOK_WORD
bool Config::_parse_route_pattern(t_tokens& tokens) {
}

// route ::= 'location' route_pattern '{' directives '}'
bool Config::_parse_route(t_tokens& tokens) {
	_parse_route_pattern(tokens);
}

// server_configs ::= [ directive | route ] server_configs?
bool Config::_parse_server_configs(t_tokens& tokens) {
	_parse_directive(tokens);
	_parse_route(tokens);
	_parse_server_configs(tokens);
}

// server ::= '{' server_configs '}'
bool Config::_parse_server(t_tokens& tokens) {
	_parse_server_configs(tokens);
}

// server_list ::= server server_list?
bool Config::_parse_server_list(t_tokens& tokens) {
	_parse_server(tokens);
	_parse_server_list(tokens);
}

// arguments ::= argument arguments?
// argument ::= TOK_WORD
bool Config::_parse_arguments(t_tokens& tokens) {
	if (!_expect(tokens, TOK_WORD))
		return false;
	token_type argument = tokens.front().first;
	if (argument == TOK_SEMICOLON)
		return true;
	else if (argument == TOK_OPENING_BRACE)
		return true;
	else if (argument == TOK_CLOSING_BRACE)
		return true;
	_parse_arguments(tokens);
}

// directive ::= directive_name arguments ';'
// directive_name ::= TOK_WORD
bool Config::_parse_directive(t_tokens& tokens) {
	if (!_accept(tokens, TOK_WORD))
		return false;

	if (!_parse_arguments(tokens))
		return false;

	if (!_accept(tokens, TOK_SEMICOLON))
		return false;
	return true;
}

// http_block ::= '{' server_list '}'
bool Config::_parse_http_block(t_tokens& tokens) {
	_parse_server_list(tokens);
}

// directives ::= directive directives?
bool Config::_parse_directives(t_tokens& tokens) {
	if (_expect())
	if (tokens.front().second == "http")
		return true;
	return _parse_directive(tokens) && _parse_directives(tokens);
}

// config ::= directives? http_block
bool Config::_parse_config(t_tokens& tokens) {
	return _parse_directives(tokens) && _parse_http_block(tokens);
}
*/

t_tokens Config::_lex_config(std::ifstream& config) {
	t_tokens tokens;
	char c;
	bool create_new_token = true;
	
	while (config.get(c)) {
		if (isspace(c)) {
			create_new_token = true;
			if (tokens.back().second == ";")
				tokens.back().first = TOK_SEMICOLON;
			else if (tokens.back().second == "{")
				tokens.back().first = TOK_OPENING_BRACE;
			else if (tokens.back().second == "}")
				tokens.back().first = TOK_CLOSING_BRACE;
			else if (tokens.back().second == "")
				tokens.back().first = TOK_EOF;
			else
				tokens.back().first = TOK_WORD;
			continue;
		}
		if (create_new_token) {
			tokens.push_back(_new_token(TOK_UNKNOWN, ""));
			create_new_token = false;
		}
		tokens.back().second += c;
	}
	return tokens;
}

/*
void Config::_parse() {
	std::ifstream config(_configPath.c_str());
	if (!config.good())
		throw runtime_error(Errors::Config::OpeningError);
	// _accessLog = "log/server1-acc.log";
	// _errorLog = "log/server1-err.log";
	// _maxBodySize = 4096;
	// _root = "www";
	// _errorPage = "www/404.html";
	// _enableDirListing = true;
	// _indexFiles.push_back("index.html");
	// _indexFiles.push_back("index.htm");
	// _indexFiles.push_back("webserv-index.html");
	// _cgiHandlers["py"] = CgiHandler("py");
	// _cgiHandlers["php"] = CgiHandler("php");
	// _serverConfigs.push_back(ServerConfig(this->_accessLog, this->_errorLog, this->_maxBodySize, this->_root, this->_errorPage, this->_enableDirListing, this->_indexFiles, this->_cgiHandlers, "Server1", "127.0.0.1", 8080));
	// _maxWorkers = 8;

	t_tokens tokens = _lex_config(config);
	_parse_config(tokens);
}
*/
