#pragma once /* Config.hpp */

#include <map>
#include <utility>
#include <vector>
#include <deque>
#include <string>

typedef std::vector<std::string> Arguments;
typedef std::pair<std::string, Arguments> Directive;
typedef std::map<std::string, Arguments> Directives;
typedef std::pair<std::string, Directives> RouteCtx;
typedef std::vector<RouteCtx> RouteCtxs;
typedef std::pair<Directives, RouteCtxs> ServerCtx;
typedef std::vector<ServerCtx> ServerCtxs;
typedef std::pair<Directives, ServerCtxs> Config;

enum TokenType {
	TOK_SEMICOLON,
	TOK_OPENING_BRACE,
	TOK_CLOSING_BRACE,
	TOK_WORD,
	TOK_EOF,
	TOK_UNKNOWN,
};
typedef enum TokenType TokenType;

typedef std::pair<TokenType, std::string> Token;
typedef std::deque<Token> Tokens;

std::string readConfig(std::string configPath);
Config parseConfig(std::string rawConfig);
