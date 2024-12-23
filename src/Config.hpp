// <GENERATED>
#pragma once /* Config.hpp */

#include <deque> /* std::deque */
#include <fstream> /* std::ifstream */
#include <string> /* std::string */
#include <iostream> /* std::ostream */
#include <map> /* std::map */

#include "repr.hpp" /* repr<T> */
#include "helper.hpp"
#include "BaseConfig.hpp"
#include "ServerConfig.hpp"
#include "CgiHandler.hpp"

using std::map;
using std::pair;
using std::string;
using std::ostream;
using std::deque;

typedef enum {
	SEMICOLON,
	OPENING_BRACE,
	CLOSING_BRACE,
	WORD,
	EOF_TOK,
	UNKNOWN,
} token_type;

typedef pair<token_type, string> t_token;
typedef deque<t_token> t_tokens;

class Config : public BaseConfig {
public:
	// <generated>
		~Config(); // destructor; consider virtual if it's a base class
		Config(); // default constructor
		Config(const string&);
		Config(	const string&,
				const string&,
				unsigned int,
				const string&,
				const string&,
				bool,
				const vector<string>&,
				const map<string, CgiHandler>&,
				const string&,
				const vector<ServerConfig>&,
				unsigned int); // serializing constructor
		Config(const Config&); // copy constructor
		Config& operator=(Config); // copy-assignment operator
		void swap(Config&); // copy-swap idiom
		string repr() const; // return string-serialized version of the object
		operator string() const; // convert object to string

		const string& getConfigPath() const;
		const vector<ServerConfig>& getServerConfigs() const;
		unsigned int getMaxWorkers() const;

		void setConfigPath(const string&);
		void setServerConfigs(const vector<ServerConfig>&);
		void setMaxWorkers(unsigned int);

		template <typename T>
		Config(const T& type, DeleteOverload = 0); // disallow accidental casting/conversion
	// </generated>
protected:
	string _configPath;
	vector<ServerConfig> _serverConfigs;
	unsigned int _maxWorkers;
private:
	t_tokens lex_config(std::ifstream&);
	void parse();
	bool parse_config(t_tokens&);
	bool parse_directives(t_tokens&);
	bool parse_http_block(t_tokens&);
	bool parse_directive(t_tokens&);
	bool parse_arguments(t_tokens&);
	bool parse_server_list(t_tokens&);
	bool parse_server(t_tokens&);
	bool parse_server_configs(t_tokens&);
	bool parse_route(t_tokens&);
	bool parse_route_pattern(t_tokens&);
	bool accept(t_tokens&, token_type);
	bool expect(t_tokens&, token_type);
	void unexpected(t_tokens&);
	t_token new_token(token_type, const string&);

	unsigned int _id;
	static unsigned int _idCntr;
};

template <> inline string repr(const Config& value) { return value.repr(); }
void swap(Config&, Config&) /* noexcept */;
ostream& operator<<(ostream&, const Config&);
// </GENERATED>
