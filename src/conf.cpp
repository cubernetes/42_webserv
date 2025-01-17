#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept> /* std::runtime_error */

#include "conf.hpp"
#include "Errors.hpp"

using std::string;
using std::runtime_error;

static void update_if_not_exists(t_directives& directives, const string& directive, const string& value) {
	if (directives.find(directive) == directives.end())
		directives[directive] = t_arguments(1, value);
}

static void update_if_not_exists_vec(t_directives& directives, const string& directive, const t_arguments& value) {
	if (directives.find(directive) == directives.end())
		directives[directive] = value;
}

static void populate_default_global_directives(t_directives& directives) {
	update_if_not_exists(directives, "pid", "run/webserv.pid");
	update_if_not_exists(directives, "index", "index.html");
	update_if_not_exists(directives, "worker_processes", "auto");
	update_if_not_exists(directives, "root", "www/html");
	update_if_not_exists(directives, "listen", "127.0.0.1:80");
}

static void take_from_default(t_directives& directives, t_directives& global_directives, const string& directive, const string& value) {
	if (global_directives.find(directive) == global_directives.end())
		update_if_not_exists(directives, directive, value);
	else
		update_if_not_exists_vec(directives, directive, global_directives[directive]);
}

static void populate_default_server_directives(t_directives& directives, t_directives& global_directives) {
	take_from_default(directives, global_directives, "index", "index.html");
	take_from_default(directives, global_directives, "worker_processes", "auto");
	take_from_default(directives, global_directives, "root", "www/html");
	take_from_default(directives, global_directives, "listen", "127.0.0.1:80");
	update_if_not_exists(directives, "server_name", "");
	update_if_not_exists(directives, "access_log", "log/access.log");
	update_if_not_exists(directives, "error_log", "log/error.log");
}

static void populate_default_route_directives(t_directives& directives, t_directives& server_directives) {
	take_from_default(directives, server_directives, "index", "index.html");
	take_from_default(directives, server_directives, "root", "www/html");
	update_if_not_exists(directives, "methods", "GET");
	update_if_not_exists(directives, "return", "");
}

static t_arguments parseArguments(t_tokens& tokens) {
	t_arguments arguments;
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

static t_directive parseDirective(t_tokens& tokens) {
	t_directive directive;
	if (tokens.empty() || tokens.front().second == "http")
		return directive;
	if (tokens.front().first != TOK_WORD)
		throw runtime_error(Errors::Config::ParseError);
	directive.first = tokens.front().second;
	tokens.pop_front();
	directive.second = parseArguments(tokens);
	if (tokens.empty() || tokens.front().first != TOK_SEMICOLON)
		throw runtime_error(Errors::Config::ParseError); // TODO make ParseError a function that takes tokens and says EOF or that token as an err msg
	tokens.pop_front();
	return directive;
}

static t_directives parseDirectives(t_tokens& tokens) {
	t_directives directives;
	while (true) {
		if (tokens.empty() || tokens.front().second == "location" || tokens.front().second == "}")
			break;
		t_directive directive = parseDirective(tokens);
		if (directive.first.empty())
			break;
		directives[directive.first] = directive.second;
	}
	return directives;
}

static t_route parseRoute(t_tokens& tokens) {
	t_route route;

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

static t_routes parseRoutes(t_tokens& tokens) {
	std::vector<t_route> routes;
	while (true) {
		if (tokens.empty() || tokens.front().second != "location")
			break;
		t_route route = parseRoute(tokens);
		routes.push_back(route);
	}
	return routes;
}

static t_server parseServer(t_tokens& tokens) {
	t_server server;

	if (tokens.empty() || tokens.front().second != "server")
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();
	if (tokens.empty() || tokens.front().first != TOK_OPENING_BRACE)
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();

	t_directives directives = parseDirectives(tokens);
	t_routes routes = parseRoutes(tokens);
	server = std::make_pair(directives, routes);

	if (tokens.empty() || tokens.front().first != TOK_CLOSING_BRACE)
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();

	return server;
}

static t_servers parseHttpBlock(t_tokens& tokens) {
	t_servers servers;

	if (tokens.empty() || tokens.front().second != "http")
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();
	if (tokens.empty() || tokens.front().first != TOK_OPENING_BRACE)
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();

	while (true) {
		if (tokens.empty() || tokens.front().second != "server")
			break;
		t_server server = parseServer(tokens);
		servers.push_back(server);
	}

	if (tokens.empty() || tokens.front().first != TOK_CLOSING_BRACE)
		throw runtime_error(Errors::Config::ParseError);
	tokens.pop_front();

	return servers;
}

static t_token new_token(token_type t, const string& s) {
	return t_token(t, s);
}

static t_tokens lex_config(string rawConfig) {
	t_tokens tokens;
	char c;
	char prev_c;
	bool create_new_token = true;
	
	prev_c = '\0';
	for (std::string::iterator it = rawConfig.begin(); it != rawConfig.end(); ++it) {
		c = *it;
		if (isspace(c) || c == ';' || c == '{' || c == '}' || prev_c == ';' || prev_c == '{' || prev_c == '}') {
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
			prev_c = c;
			if (isspace(c))
				continue;
		}
		prev_c = c;
		if (create_new_token) {
			tokens.push_back(new_token(TOK_UNKNOWN, ""));
			create_new_token = false;
		}
		tokens.back().second += c;
	}
	return tokens;
}

void updateDefaults(t_config& config) {
	populate_default_global_directives(config.first);
	for (t_servers::iterator it = config.second.begin(); it != config.second.end(); ++it) {
		populate_default_server_directives(it->first, config.first);
		for (t_routes::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
			populate_default_route_directives(it2->second, it->first);
		}
	}
}

void postProcess(t_config& config) {
	// TODO: update some directives, like e.g.
	// - the listen directive is more useful when the ip and port are split up (make two arguments from one argument), thankfully we only have to handle ipv4 i guess (actually not sure about that)
	// - yeah that's almost it, can't think of anything else.
}

t_config parseConfig(string rawConfig) {
	t_tokens tokens = lex_config(rawConfig);

	t_directives directives = parseDirectives(tokens);
	t_servers httpBlock = parseHttpBlock(tokens);
	t_config config = std::make_pair(directives, httpBlock);

	updateDefaults(config);

	postProcess(config);

	return config;
}
/* How to access t_config config:
# Global directives:
config.first["pid"] -> gives back vector of args
config.first["pid"][0] -> most directives have only one argument, so you'll use this pattern quite often
                          watch out that some directive _might_ have zero arguments, although I can't think of any, but the parser allows that

# Servers configs
config.second -> vector of t_server
config.second[0] -> pair of directives and routes
config.second[0].first -> directives specific for a server (order is not important, which is why t_directives is a map)
config.second[0].first["server_name"] -> how you would access the server name for a given server

## Routes
config.second[0].second -> vector of routes for a server (order is important for the routers (top-bottom)!, that's why it's a vector)
config.second[0].second[0] -> first route (if there are any, check for empty!)
config.second[0].second[0].first -> the actual route (a string), something like /web
config.second[0].second[0].second -> again, t_directives, so a map of directives similar to above
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
// TODO: Semantics to check:
// - conf must be non-empty
// - there must be at least one server
// - all directives must exist and make sense in the context
// - arguments per directive must make sense

// TODO make everything snake case or camelcase
