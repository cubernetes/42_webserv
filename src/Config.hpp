#pragma once /* Config.hpp */

#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>

using std::string;

typedef std::vector<string> Arguments;
typedef std::pair<string, Arguments> Directive;
typedef std::multimap<string, Arguments> Directives;
typedef std::pair<string, Directives> LocationCtx;
typedef std::vector<LocationCtx> LocationCtxs;
typedef std::pair<Directives, LocationCtxs> ServerCtx;
typedef std::vector<ServerCtx> ServerCtxs;
typedef std::pair<Directives, ServerCtxs> Config;

typedef std::vector<Arguments> ArgResults;

enum TokenType {
    TOK_SEMICOLON,
    TOK_OPENING_BRACE,
    TOK_CLOSING_BRACE,
    TOK_WORD,
    TOK_EOF,
    TOK_UNKNOWN,
};
typedef enum TokenType TokenType;

typedef std::pair<TokenType, string> Token;
typedef std::deque<Token> Tokens;

string readConfig(string configPath);
Config parseConfig(string rawConfig);

bool directiveExists(const Directives &directives, const string &directive);
const Arguments &getFirstDirective(const Directives &directives, const string &directive);
ArgResults getAllDirectives(const Directives &directives, const string &directive);
