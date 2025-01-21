#include <utility>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

#include "Config.hpp"
#include "Errors.hpp"
#include "Constants.hpp"
#include "Utils.hpp"
#include "DirectiveValidation.hpp"

using std::string;
using std::runtime_error;

static void updateIfNotExists(Directives& directives, const string& directive, const string& value) {
	if (directives.find(directive) == directives.end())
		directives.insert(std::make_pair(directive, VEC(string, value)));
}

static void updateIfNotExistsVec(Directives& directives, const string& directive, const Arguments& value) {
	if (directives.find(directive) == directives.end())
		directives.insert(std::make_pair(directive, value));
}

bool directiveExists(const Directives& directives, const string& directive) {
	Directives::const_iterator result_itr = directives.find(directive);
	if (result_itr == directives.end())
		return false;
	return true;
}

const Arguments& getFirstDirective(const Directives& directives, const string& directive) {
	Directives::const_iterator result_itr = directives.find(directive);
	if (result_itr == directives.end())
		throw runtime_error(Errors::MultimapIndex(directive));
	return result_itr->second;
}

std::vector<Arguments> getAllDirectives(const Directives& directives, const string& directive) {
	std::pair<Directives::const_iterator, Directives::const_iterator> result_itr = directives.equal_range(directive);
	std::vector<Arguments> allArgs;

	for (Directives::const_iterator it = result_itr.first; it != result_itr.second; ++it) {
		allArgs.push_back(it->second);
	}
	return allArgs;
}

static void takeFromDefault(Directives& directives, Directives& httpDirectives, const string& directive, const string& value) {
	if (httpDirectives.find(directive) == httpDirectives.end())
		updateIfNotExists(directives, directive, value);
	else
		updateIfNotExistsVec(directives, directive, getFirstDirective(httpDirectives, directive));
}

static void populateDefaultHttpDirectives(Directives& directives) {
	updateIfNotExists(directives, "autoindex", "off");
	updateIfNotExists(directives, "cgi_dir", "cgi-bin");
	updateIfNotExists(directives, "client_max_body_size", "1m");
	updateIfNotExists(directives, "index", "index.html");
	updateIfNotExists(directives, "root", "html");
	updateIfNotExists(directives, "upload_dir", "");
}

static void populateDefaultServerDirectives(Directives& directives, Directives& httpDirectives) {
	takeFromDefault(directives, httpDirectives, "autoindex", "off");
	takeFromDefault(directives, httpDirectives, "cgi_dir", "cgi-bin");
	takeFromDefault(directives, httpDirectives, "client_max_body_size", "1m");
	takeFromDefault(directives, httpDirectives, "index", "index.html");
	takeFromDefault(directives, httpDirectives, "root", "html");
	takeFromDefault(directives, httpDirectives, "upload_dir", "");

	updateIfNotExists(directives, "listen", "*:8000");
	updateIfNotExists(directives, "server_name", "");
}

static void populateDefaultLocationDirectives(Directives& directives, Directives& serverDirectives) {
	takeFromDefault(directives, serverDirectives, "autoindex", "off");
	takeFromDefault(directives, serverDirectives, "cgi_dir", "cgi-bin");
	takeFromDefault(directives, serverDirectives, "client_max_body_size", "1m");
	takeFromDefault(directives, serverDirectives, "index", "index.html");
	takeFromDefault(directives, serverDirectives, "root", "html");
	takeFromDefault(directives, serverDirectives, "upload_dir", "");
}

static Arguments parseArguments(Tokens& tokens) {
	Arguments arguments;
	while (!tokens.empty()) {
		if (tokens.front().first == TOK_WORD) {
			arguments.push_back(tokens.front().second);
			tokens.pop_front();
		}
		else
			break;
	}
	return arguments;
}

static Directive parseDirective(Tokens& tokens) {
	Directive directive;
	if (tokens.empty() || tokens.front().second == "http")
		return directive;
	if (tokens.front().first != TOK_WORD)
		throw runtime_error(Errors::Config::ParseError(tokens));
	directive.first = tokens.front().second;
	tokens.pop_front();
	directive.second = parseArguments(tokens);
	if (tokens.empty() || tokens.front().first != TOK_SEMICOLON)
		throw runtime_error(Errors::Config::ParseError(tokens));
	tokens.pop_front();
	return directive;
}

static Directives parseDirectives(Tokens& tokens) {
	Directives directives;
	while (true) {
		if (tokens.empty() || tokens.front().second == "location" || tokens.front().second == "server" || tokens.front().second == "}")
			break;
		Directive directive = parseDirective(tokens);
		if (directive.first.empty())
			break;
		directives.insert(directive);
	}
	return directives;
}

static LocationCtx parseLocation(Tokens& tokens) {
	LocationCtx location;

	if (tokens.empty() || tokens.front().second != "location")
		throw runtime_error(Errors::Config::ParseError(tokens));
	tokens.pop_front();

	if (tokens.empty() || tokens.front().first != TOK_WORD)
		throw runtime_error(Errors::Config::ParseError(tokens));

	location.first = tokens.front().second;
	tokens.pop_front();

	if (tokens.empty() || tokens.front().first != TOK_OPENING_BRACE)
		throw runtime_error(Errors::Config::ParseError(tokens));
	tokens.pop_front();

	location.second = parseDirectives(tokens);

	if (tokens.empty() || tokens.front().first != TOK_CLOSING_BRACE)
		throw runtime_error(Errors::Config::ParseError(tokens));
	tokens.pop_front();

	return location;
}

static LocationCtxs parseLocations(Tokens& tokens) {
	std::vector<LocationCtx> locations;
	while (true) {
		if (tokens.empty() || tokens.front().second != "location")
			break;
		LocationCtx location = parseLocation(tokens);
		locations.push_back(location);
	}
	return locations;
}

static ServerCtx parseServer(Tokens& tokens) {
	ServerCtx server;

	if (tokens.empty() || tokens.front().second != "server")
		throw runtime_error(Errors::Config::ParseError(tokens));
	tokens.pop_front();
	if (tokens.empty() || tokens.front().first != TOK_OPENING_BRACE)
		throw runtime_error(Errors::Config::ParseError(tokens));
	tokens.pop_front();

	Directives directives = parseDirectives(tokens);
	LocationCtxs locations = parseLocations(tokens);
	server = std::make_pair(directives, locations);

	if (tokens.empty() || tokens.front().first != TOK_CLOSING_BRACE)
		throw runtime_error(Errors::Config::ParseError(tokens));
	tokens.pop_front();

	return server;
}

static ServerCtxs parseServers(Tokens& tokens) {
	ServerCtxs servers;

	while (true) {
		if (tokens.empty() || tokens.front().second != "server")
			break;
		ServerCtx server = parseServer(tokens);
		servers.push_back(server);
	}

	return servers;
}

static string removeComments(const string& rawConfig) {
	std::istringstream iss(rawConfig);
	string line;
	string cleanedConfig;

	while (std::getline(iss, line)) {
		string newLine;
		for (string::iterator c = line.begin(); c != line.end(); ++c) {
			if (*c != Constants::commentSymbol) {
				newLine += *c;
				continue;
			}
			break;
		}
		cleanedConfig += newLine + '\n';
	}
	return cleanedConfig;
}

static Token newToken(TokenType t, const string& s) {
	return Token(t, s);
}

static void updateTokenType(Tokens& tokens) {
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
}

static Tokens lexConfig(string rawConfig) {
	Tokens tokens;
	char c;
	char prevC;
	bool createNewToken = true;
	
	rawConfig = removeComments(rawConfig);
	prevC = '\0';
	for (std::string::iterator it = rawConfig.begin(); it != rawConfig.end(); ++it) {
		c = *it;
		if (isspace(c) || c == ';' || c == '{' || c == '}' || prevC == ';' || prevC == '{' || prevC == '}') {
			if (!tokens.empty()) {
				createNewToken = true;
				updateTokenType(tokens);
			}
			prevC = c;
			if (isspace(c))
				continue;
		}
		prevC = c;
		if (createNewToken) {
			tokens.push_back(newToken(TOK_UNKNOWN, ""));
			createNewToken = false;
		}
		tokens.back().second += c;
	}
	return tokens;
}

static void updateDefaults(Config& config) {
	populateDefaultHttpDirectives(config.first);
	for (ServerCtxs::iterator server = config.second.begin(); server != config.second.end(); ++server) {
		populateDefaultServerDirectives(server->first, config.first);
		for (LocationCtxs::iterator location = server->second.begin(); location != server->second.end(); ++location) {
			populateDefaultLocationDirectives(location->second, server->first);
		}
	}
}

static void checkDirectives(Config& config) {
	checkHttpDirectives(config.first);
	for (ServerCtxs::iterator server = config.second.begin(); server != config.second.end(); ++server) {
		checkServerDirectives(server->first);
		for (LocationCtxs::iterator location = server->second.begin(); location != server->second.end(); ++location) {
			checkLocationDirectives(location->second);
		}
	}
}

Config parseConfig(string rawConfig) {
	Tokens tokens = lexConfig(rawConfig);

	if (tokens.empty())
		throw runtime_error(Errors::Config::ZeroServers());

	Directives directives = parseDirectives(tokens);
	ServerCtxs servers = parseServers(tokens);
	if (!tokens.empty())
		throw runtime_error(Errors::Config::ParseError(tokens));
	Config config = std::make_pair(directives, servers);

	updateDefaults(config);

	checkDirectives(config);

	if (config.second.empty())
		throw runtime_error(Errors::Config::ZeroServers());

	return config;
}
/* How to access Config config:
# Http directives:
config.first["pid"] -> gives back vector of args
config.first["pid"][0] -> most directives have only one argument, so you'll use this pattern quite often
                          watch out that some directive _might_ have zero arguments, although I can't think of any, but the parser allows that

# ServerCtxs configs
config.second -> vector of ServerCtx
config.second[0] -> pair of directives and locations
config.second[0].first -> directives specific for a server (order is not important, which is why Directives is a map)
config.second[0].first["server_name"] -> how you would access the server name for a given server

## Locations
config.second[0].second -> vector of locations for a server (order is important for the locations (top-bottom)!, that's why it's a vector)
config.second[0].second[0] -> first location (if there are any, check for empty!)
config.second[0].second[0].first -> the actual location (a string), something like /web
config.second[0].second[0].second -> again, Directives, so a map of directives similar to above
config.second[0].second[0].second["limit_except"] -> which methods does this location support? mutliple arguments!
config.second[0].second[0].second["limit_except"][0] == "GET" -> check if the first allowed method for this location is "GET"


# Example JSON (might want to add more keys, instead of just plain lists, but let's keep it MVP)
[ {
    "index": [ "index.html" ],
    "listen": [ "127.0.0.1:80" ],
    "pid": [ "run/webserv.pid" ],
    "root": [ "html" ],
    "worker_processes": [ "auto" ]
  },
  [ [ {
        "access_log": [ "log/server1_access.log" ],
        "error_log": [ "log/server1_error.log" ],
        "index": [ "index.html" ],
        "listen": [ "127.0.0.1:8080" ],
        "root": [ "html/server1" ],
        "server_name": [ "server1" ],
        "worker_processes": [ "auto" ]
      },
      [ [ "/",
          {
            "index": [ "index.html" ],
            "limit_except": [ "GET" ],
            "return": [ "" ],
            "root": [ "html/server1" ]
          }
      ] ]
    ],
    [ {
        "access_log": [ "log/server2_access.log" ],
        "error_log": [ "log/server2_error.log" ],
        "index": [ "index.html" ],
        "listen": [ "127.0.0.1:8081" ],
        "root": [ "html/server2" ],
        "server_name": [ "server2" ],
        "worker_processes": [ "auto" ]
      },
      [ [
          "/",
          {
            "index": [ "index.html" ],
            "limit_except": [ "GET" ],
            "return": [ "" ],
            "root": [ "html/server2" ],
            "try_files": [ "$uri", "$uri/", "=404" ]
          }
] ] ] ] ]
*/

string readConfig(string configPath) {
	std::ifstream is(configPath.c_str());
	if (!is.good())
		throw runtime_error(Errors::Config::OpeningError(configPath));
	std::stringstream ss;
	ss << is.rdbuf();
	return ss.str();
}
