#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "repr.hpp"

typedef std::map<std::string, std::string> t_directives;
typedef std::map<std::string, std::string> t_route;
typedef std::vector<t_route> t_routes;
typedef std::pair<t_directives, t_routes> t_server;
typedef std::vector<t_server> t_servers;
typedef std::pair<t_directives, t_servers> t_config;

void populate_default_global_directives(t_directives& directives) {
	directives["pid"] = "run/webserv.pid";
	directives["index"] = "index.html";
	directives["worker_processes"] = "auto";
	directives["root"] = "www/html";
	directives["listen"] = "127.0.0.1:80";
}

void take_from_default(t_directives& directives, t_directives& global_directives, const std::string& directive, const std::string& value) {
	directives[directive] = global_directives[directive].empty() ? value : global_directives[directive];
}

void populate_default_server_directives(t_directives& directives, t_directives& global_directives) {
	take_from_default(directives, global_directives, "index", "index.html");
	take_from_default(directives, global_directives, "worker_processes", "auto");
	take_from_default(directives, global_directives, "root", "www/html");
	take_from_default(directives, global_directives, "listen", "127.0.0.1:80");
	directives["server_name"] = "";
	directives["access_log"] = "log/access.log";
	directives["error_log"] = "log/error.log";
}

void default_route(t_route& route) {
}

int main() {
	t_config config;
	populate_default_global_directives(config.first);
	t_server server;
	populate_default_server_directives(server.first, config.first);
	config.second.push_back(server);
	std::cout << repr(config) << '\n';
	return 0;
}
