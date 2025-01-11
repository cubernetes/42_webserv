#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "repr.hpp"
#include "conf.hpp"

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

void populate_default_route_directives(t_directives& directives, t_directives& server_directives) {
	take_from_default(directives, server_directives, "index", "index.html");
	take_from_default(directives, server_directives, "root", "www/html");
	directives["methods"] = "GET";
	directives["return"] = "";
}

void default_route(t_route& route) {
	route.first = "";
	populate_default_route_directives(route.second, );
}

int main() {
	t_config config;
	populate_default_global_directives(config.first);

	t_server server;
	populate_default_server_directives(server.first, config.first);

	t_route route;
	default_route(route);
	server.second.push_back(route);

	config.second.push_back(server);

	std::cout << repr(config, true) << '\n';
	return 0;
}
