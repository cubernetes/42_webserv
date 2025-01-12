#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

typedef std::map<std::string, std::string> t_directives;
typedef std::pair<std::string, t_directives> t_route;
typedef std::vector<t_route> t_routes;
typedef std::pair<t_directives, t_routes> t_server;
typedef std::vector<t_server> t_servers;
typedef std::pair<t_directives, t_servers> t_config;

std::string readConfig(std::string configPath);
t_config parseConfig(std::string rawConfig);
