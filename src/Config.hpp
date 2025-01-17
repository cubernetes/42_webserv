#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <deque>

typedef std::vector<std::string> t_arguments;
typedef std::pair<std::string, t_arguments> t_directive;
typedef std::map<std::string, t_arguments> t_directives;
typedef std::pair<std::string, t_directives> t_route;
typedef std::vector<t_route> t_routes;
typedef std::pair<t_directives, t_routes> t_server;
typedef std::vector<t_server> t_servers;
typedef std::pair<t_directives, t_servers> t_config;

enum e_token_type {
	TOK_SEMICOLON,
	TOK_OPENING_BRACE,
	TOK_CLOSING_BRACE,
	TOK_WORD,
	TOK_EOF,
	TOK_UNKNOWN,
};
typedef enum e_token_type token_type;

typedef std::pair<token_type, std::string> t_token;
typedef std::deque<t_token> t_tokens;

std::string readConfig(std::string configPath);
t_config parseConfig(std::string rawConfig);
