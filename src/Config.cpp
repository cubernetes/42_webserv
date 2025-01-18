#include <utility>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

#include "Config.hpp"
#include "Errors.hpp"

using std::string;
using std::runtime_error;

static void updateIfNotExists(Directives& directives, const string& directive, const string& value) {
	if (directives.find(directive) == directives.end())
		directives[directive] = Arguments(1, value);
}

static void updateIfNotExistsVec(Directives& directives, const string& directive, const Arguments& value) {
	if (directives.find(directive) == directives.end())
		directives[directive] = value;
}

static void populateDefaultGlobalDirectives(Directives& directives) {
	updateIfNotExists(directives, "pid", "run/webserv.pid");
	updateIfNotExists(directives, "index", "index.html");
	updateIfNotExists(directives, "worker_processes", "auto");
	updateIfNotExists(directives, "root", "www/html");
	updateIfNotExists(directives, "listen", "127.0.0.1:80");
}

static void takeFromDefault(Directives& directives, Directives& globalDirectives, const string& directive, const string& value) {
	if (globalDirectives.find(directive) == globalDirectives.end())
		updateIfNotExists(directives, directive, value);
	else
		updateIfNotExistsVec(directives, directive, globalDirectives[directive]);
}

static void populateDefaultServerDirectives(Directives& directives, Directives& globalDirectives) {
	takeFromDefault(directives, globalDirectives, "index", "index.html");
	takeFromDefault(directives, globalDirectives, "worker_processes", "auto");
	takeFromDefault(directives, globalDirectives, "root", "www/html");
	takeFromDefault(directives, globalDirectives, "listen", "127.0.0.1:80");
	updateIfNotExists(directives, "server_name", "");
	updateIfNotExists(directives, "access_log", "log/access.log");
	updateIfNotExists(directives, "error_log", "log/error.log");
}

static void populateDefaultRouteDirectives(Directives& directives, Directives& serverDirectives) {
	takeFromDefault(directives, serverDirectives, "index", "index.html");
	takeFromDefault(directives, serverDirectives, "root", "www/html");
	updateIfNotExists(directives, "methods", "GET");
	updateIfNotExists(directives, "return", "");
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
		throw runtime_error(Errors::Config::ParseError);
	directive.first = tokens.front().second;
	tokens.pop_front();
	directive.second = parseArguments(tokens);
	if (tokens.empty() || tokens.front().first != TOK_SEMICOLON)
		throw runtime_error(Errors::Config::ParseError); // TODO: @timo: make ParseError a function that takes tokens and says EOF or that token as an err msg
	tokens.pop_front();
	return directive;
}

static Directives parseDirectives(Tokens& tokens) {
	Directives directives;
	while (true) {
		if (tokens.empty() || tokens.front().second == "location" || tokens.front().second == "}")
			break;
		Directive directive = parseDirective(tokens);
		if (directive.first.empty())
			break;
		directives[directive.first] = directive.second;
	}
	return directives;
}

static RouteCtx parseRoute(Tokens& tokens) {
	RouteCtx route;

	if (tokens.empty() || tokens.front().second != "location")
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();

	if (tokens.empty() || tokens.front().first != TOK_WORD)
		throw runtime_error(Errors::Config::ParseError);

	route.first = tokens.front().second;
	tokens.pop_front();

	if (tokens.empty() || tokens.front().first != TOK_OPENING_BRACE)
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();

	route.second = parseDirectives(tokens);

	if (tokens.empty() || tokens.front().first != TOK_CLOSING_BRACE)
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();

	return route;
}

static RouteCtxs parseRoutes(Tokens& tokens) {
	std::vector<RouteCtx> routes;
	while (true) {
		if (tokens.empty() || tokens.front().second != "location")
			break;
		RouteCtx route = parseRoute(tokens);
		routes.push_back(route);
	}
	return routes;
}

static ServerCtx parseServer(Tokens& tokens) {
	ServerCtx server;

	if (tokens.empty() || tokens.front().second != "server")
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();
	if (tokens.empty() || tokens.front().first != TOK_OPENING_BRACE)
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();

	Directives directives = parseDirectives(tokens);
	RouteCtxs routes = parseRoutes(tokens);
	server = std::make_pair(directives, routes);

	if (tokens.empty() || tokens.front().first != TOK_CLOSING_BRACE)
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();

	return server;
}

static ServerCtxs parseHttpBlock(Tokens& tokens) {
	ServerCtxs servers;

	if (tokens.empty() || tokens.front().second != "http")
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();
	if (tokens.empty() || tokens.front().first != TOK_OPENING_BRACE)
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();

	while (true) {
		if (tokens.empty() || tokens.front().second != "server")
			break;
		ServerCtx server = parseServer(tokens);
		servers.push_back(server);
	}

	if (tokens.empty() || tokens.front().first != TOK_CLOSING_BRACE)
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();

	return servers;
}

static Token newToken(TokenType t, const string& s) {
	return Token(t, s);
}

static Tokens lexConfig(string rawConfig) {
	Tokens tokens;
	char c;
	char prevC;
	bool createNewToken = true;
	
	prevC = '\0';
	for (std::string::iterator it = rawConfig.begin(); it != rawConfig.end(); ++it) {
		c = *it;
		if (isspace(c) || c == ';' || c == '{' || c == '}' || prevC == ';' || prevC == '{' || prevC == '}') {
			createNewToken = true;
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

void updateDefaults(Config& config) {
	populateDefaultGlobalDirectives(config.first);
	for (ServerCtxs::iterator it = config.second.begin(); it != config.second.end(); ++it) {
		populateDefaultServerDirectives(it->first, config.first);
		for (RouteCtxs::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
			populateDefaultRouteDirectives(it2->second, it->first);
		}
	}
}

void postProcess(Config& config) {
	// TODO: @timo: update some directives, like e.g.
	// - the listen directive is more useful when the ip and port are split up (make two arguments from one argument), thankfully we only have to handle ipv4 i guess (actually not sure about that)
	// - yeah that's almost it, can't think of anything else.
	(void)config;
}

Config parseConfig(string rawConfig) {
	Tokens tokens = lexConfig(rawConfig);

	Directives directives = parseDirectives(tokens);
	ServerCtxs httpBlock = parseHttpBlock(tokens);
	if (!tokens.empty())
		throw runtime_error(Errors::Config::ParseError);
	Config config = std::make_pair(directives, httpBlock);

	updateDefaults(config);

	postProcess(config);

	return config;
}
/* How to access Config config:
# Global directives:
config.first["pid"] -> gives back vector of args
config.first["pid"][0] -> most directives have only one argument, so you'll use this pattern quite often
                          watch out that some directive _might_ have zero arguments, although I can't think of any, but the parser allows that

# ServerCtxs configs
config.second -> vector of ServerCtx
config.second[0] -> pair of directives and routes
config.second[0].first -> directives specific for a server (order is not important, which is why Directives is a map)
config.second[0].first["server_name"] -> how you would access the server name for a given server

## Routes
config.second[0].second -> vector of routes for a server (order is important for the routers (top-bottom)!, that's why it's a vector)
config.second[0].second[0] -> first route (if there are any, check for empty!)
config.second[0].second[0].first -> the actual route (a string), something like /web
config.second[0].second[0].second -> again, Directives, so a map of directives similar to above
config.second[0].second[0].second["methods"] -> which methods does this route support? mutliple arguments!
config.second[0].second[0].second["methods"][0] == "GET" -> check if the first allowed method for this route is "GET"


# Example JSON (might want to add more keys, instead of just plain lists, but let's keep it MVP)
[ {
    "index": [ "index.html" ],
    "listen": [ "127.0.0.1:80" ],
    "pid": [ "run/webserv.pid" ],
    "root": [ "www/html" ],
    "worker_processes": [ "auto" ]
  },
  [ [ {
        "access_log": [ "log/server1_access.log" ],
        "error_log": [ "log/server1_error.log" ],
        "index": [ "index.html" ],
        "listen": [ "127.0.0.1:8080" ],
        "root": [ "www/server1" ],
        "server_name": [ "server1" ],
        "worker_processes": [ "auto" ]
      },
      [ [ "/",
          {
            "index": [ "index.html" ],
            "methods": [ "GET" ],
            "return": [ "" ],
            "root": [ "www/server1" ]
          }
      ] ]
    ],
    [ {
        "access_log": [ "log/server2_access.log" ],
        "error_log": [ "log/server2_error.log" ],
        "index": [ "index.html" ],
        "listen": [ "127.0.0.1:8081" ],
        "root": [ "www/server2" ],
        "server_name": [ "server2" ],
        "worker_processes": [ "auto" ]
      },
      [ [
          "/",
          {
            "index": [ "index.html" ],
            "methods": [ "GET" ],
            "return": [ "" ],
            "root": [ "www/server2" ],
            "try_files": [ "$uri", "$uri/", "=404" ]
          }
] ] ] ] ]
*/

string readConfig(string configPath) {
	std::ifstream is(configPath.c_str());
	if (!is.good())
		throw runtime_error(Errors::Config::OpeningError);
	std::stringstream ss;
	ss << is.rdbuf();
	return ss.str();
}
// TODO: @timo: Semantics to check:
// - conf must be non-empty
// - there must be at least one server
// - all directives must exist and make sense in the context
// - arguments per directive must make sense
