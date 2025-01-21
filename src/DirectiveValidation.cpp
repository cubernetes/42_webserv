#include <map>
#include <vector>
#include <utility>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <algorithm>
#include <climits>
#include <arpa/inet.h>

#include "Config.hpp"
#include "Errors.hpp"
#include "Constants.hpp"
#include "Utils.hpp"

#define CHECKFN_HTTP(checkFunction) \
	if (checkFunction("http", directive, arguments) && ++counts[directive] <= 1) \
		continue

#define CHECKFN_SERVER(checkFunction) \
	if (checkFunction("server", directive, arguments) && ++counts[directive] <= 1) \
		continue

#define CHECKFN_LOCATION(checkFunction) \
	if (checkFunction("location", directive, arguments) && ++counts[directive] <= 1) \
		continue

using std::map;
using std::vector;
using std::pair;
using std::string;
using std::runtime_error;

// (max < 0 -> no maximum
static inline void ensureArity(const string& ctx, const string& directive, const Arguments& arguments, int min, int max) {
	int ac = static_cast<int>(arguments.size());
	if (ac >= min && (ac <= max || max < 0))
		return;
	throw runtime_error(Errors::Config::InvalidDirectiveArgumentCount(ctx, directive, ac, min, max));
}

static inline void ensureOneOfStrings(const string& ctx, const string& directive, const string& argument, const Arguments& possibilities) {
	if (std::find(possibilities.begin(), possibilities.end(), argument) == possibilities.end()) {
		throw runtime_error(Errors::Config::InvalidDirectiveArgument(ctx, directive, argument, possibilities));
	}
}

static inline void ensureNotEmpty(const string& ctx, const string& directive, const string& argument) {
	if (argument.empty())
		throw runtime_error(Errors::Config::DirectiveArgumentEmpty(ctx, directive));
}

// was only needed for the int argument to the worker_processes directive, which is not needed anymore
static inline void ensureInt(const string& ctx, const string& directive, const string& argument) {
	ensureNotEmpty(ctx, directive, argument);
	string::const_iterator c = argument.begin();
	while (c != argument.end() && std::isdigit(*c)) ++c;
	if (c == argument.end())
		return;
	throw runtime_error(Errors::Config::DirectiveArgumentNotNumeric(ctx, directive, argument));
}

static inline void ensureValidSize(const string& ctx, const string& directive, const string& argument) {
	ensureNotEmpty(ctx, directive, argument);
	string::const_iterator c = argument.begin();
	while (c != argument.end() && std::isdigit(*c)) ++c;
	if (c == argument.end())
		return;
	if (std::strchr("kmgt", std::tolower(*c)) && c + 1 == argument.end())
		return;
	throw runtime_error(Errors::Config::DirectiveInvalidSizeArgument(ctx, directive, argument));
}

static inline void ensureOnOff(const string& ctx, const string& directive, const string& argument) {
	if (argument == "on" || argument == "yes" || argument == "true")
		return;
	if (argument == "off" || argument == "no" || argument == "false")
		return;
	throw runtime_error(Errors::Config::DirectiveInvalidBooleanArgument(ctx, directive, argument));
}

// only allow official status codes, might even prune this list even further
// https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
static inline void ensureStatusCode(const string& ctx, const string& directive, const string& argument) {
	static const char *validCodesArr[] = {
		"100", "101", "102", "103",
		"200", "201", "202", "203", "204", "205", "206", "207", "208", "226",
		"300", "301", "302", "303", "304", "305", "306", "307", "308",
		"400", "401", "402", "403", "404", "405", "406", "407", "408", "409",
		"410", "411", "412", "413", "414", "415", "416", "417", "418", "421",
		"422", "423", "424", "425", "426", "428", "429", "431", "451",
		"500", "501", "502", "503", "504", "505", "506", "507", "508", "510", "511"
	};
	static const vector<string> validCodes(validCodesArr, validCodesArr + sizeof(validCodesArr) / sizeof(validCodesArr[0]));
	if (std::find(validCodes.begin(), validCodes.end(), argument) != validCodes.end())
		return;
	throw runtime_error(Errors::Config::DirectiveInvalidStatusCodeArgument(ctx, directive, argument));
}

static inline void ensureValidHost(const string& ctx, const string& directive, string& argument) {
	if (argument == "*") {
		argument = "0.0.0.0";
		return;
	}
	char buf[16];
	if (inet_aton(argument.c_str(), NULL) == 1)
		return;
	if (inet_pton(AF_INET6, argument.c_str(), &buf) == 1)
		throw runtime_error(Errors::Config::DirectiveIPv6NotSupported(ctx, directive));
	throw runtime_error(Errors::Config::DirectiveInvalidIpAddressArgument(ctx, directive, argument));
}

static inline void ensureValidPort(const string& ctx, const string& directive, const string& argument) {
	char *end;
	long l;

	errno = 0;
	l = strtol(argument.c_str(), &end, 10);
	if ((errno == ERANGE && l == LONG_MAX) || l > Constants::highestPort)
		throw runtime_error(Errors::Config::DirectiveArgumentPortNumberTooHigh(ctx, directive, argument));
	if ((errno == ERANGE && l == LONG_MIN) || l < 1)
		throw runtime_error(Errors::Config::DirectiveArgumentPortNumberTooLow(ctx, directive, argument));
	if (argument.empty() || *end != '\0')
		throw runtime_error(Errors::Config::DirectiveArgumentInvalidPort(ctx, directive, argument));
}


static inline bool checkIndex(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "index") {
		ensureArity(ctx, directive, arguments, 1, -1);
		for (Arguments::const_iterator argument = arguments.begin(); argument != arguments.end(); ++argument) {
			ensureNotEmpty(ctx, directive, *argument);
		}
		return true;
	}
	return false;
}

static inline bool checkRoot(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "root") {
		ensureArity(ctx, directive, arguments, 1, 1);
		ensureNotEmpty(ctx, directive, arguments[0]);
		return true;
	}
	return false;
}

static inline Arguments processIPv4Address(const string& host) {
	string ipAddress;
	string port;
	Arguments newArgs;
	string::size_type pos;
	if ((pos = host.rfind(':')) != string::npos) {
		ipAddress = host.substr(0, pos);
		port = host.substr(pos + 1);
	} else if ((pos = host.find('.')) != string::npos) {
		ipAddress = host;
		port = Constants::defaultPort;
	} else {
		ipAddress = Constants::defaultAddress;
		port = host;
	}
	newArgs.push_back(ipAddress);
	newArgs.push_back(port);
	return newArgs;
}

static inline bool checkListen(const string& ctx, const string& directive, Arguments& arguments) {
	if (directive == "listen") {
		ensureArity(ctx, directive, arguments, 1, 1);
		Arguments ipAndPort = processIPv4Address(arguments[0]);
		ensureValidHost(ctx, directive, ipAndPort[0]);
		ensureValidPort(ctx, directive, ipAndPort[1]);
		arguments = ipAndPort;
		return true;
	}
	return false;
}

static inline bool checkServerName(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "server_name") {
		ensureArity(ctx, directive, arguments, 1, -1);
		return true;
	}
	return false;
}

static inline bool checkClientMaxBodySize(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "client_max_body_size") {
		ensureArity(ctx, directive, arguments, 1, 1);
		ensureValidSize(ctx, directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkLimitExcept(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "limit_except") {
		ensureArity(ctx, directive, arguments, 1, -1);
		for (Arguments::const_iterator argument = arguments.begin(); argument != arguments.end(); ++argument) {
			ensureOneOfStrings(ctx, directive, *argument, VEC(string, "GET", "POST", "DELETE"));
		}
		return true;
	}
	return false;
}

static inline bool checkAlias(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "alias") {
		ensureArity(ctx, directive, arguments, 1, 1);
		ensureNotEmpty(ctx, directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkReturn(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "return") {
		ensureArity(ctx, directive, arguments, 2, 2);
		ensureStatusCode(ctx, directive, arguments[0]);
		ensureNotEmpty(ctx, directive, arguments[1]); // TODO: @timo: ensure it's a url starting with http or https or a location
		return true;
	}
	return false;
}

static inline bool checkAutoindex(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "autoindex") {
		ensureArity(ctx, directive, arguments, 1, 1);
		ensureOnOff(ctx, directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkErrorPage(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "error_page") {
		ensureArity(ctx, directive, arguments, 1, -1);
		Arguments::const_iterator argument = arguments.begin(), before = --arguments.end();
		for (; argument != before; ++argument) {
			ensureStatusCode(ctx, directive, *argument);
		}
		ensureNotEmpty(ctx, directive, *(++argument));
		return true;
	}
	return false;
}

static inline bool checkCgiExt(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "cgi_ext") {
		ensureArity(ctx, directive, arguments, 1, 2);
		if (arguments.size() == 2)
			ensureNotEmpty(ctx, directive, arguments[1]);
		return true;
	}
	return false;
}

static inline bool checkCgiDir(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "cgi_dir") {
		ensureArity(ctx, directive, arguments, 1, 1);
		ensureNotEmpty(ctx, directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkUploadDir(const string& ctx, const string& directive, const Arguments& arguments) {
	if (directive == "upload_dir") {
		ensureArity(ctx, directive, arguments, 1, 1);
		// ensureNotEmpty(directive, arguments[0]); // actually it may be empty, signifying to the CGI application there's nothing to upload
		return true;
	}
	return false;
}

void checkHttpDirectives(Directives& directives) {
	map<string, unsigned int> counts;
	for (Directives::iterator kv = directives.begin(); kv != directives.end(); ++kv) {
		const string& directive = kv->first;
		Arguments& arguments = kv->second;
		CHECKFN_HTTP(checkAutoindex);
		CHECKFN_HTTP(checkCgiDir);
		CHECKFN_HTTP(checkCgiExt);
		CHECKFN_HTTP(checkClientMaxBodySize);
		CHECKFN_HTTP(checkErrorPage);
		CHECKFN_HTTP(checkIndex);
		CHECKFN_HTTP(checkRoot);
		CHECKFN_HTTP(checkRoot);
		CHECKFN_HTTP(checkUploadDir);
		if (counts[directive] > 1)
			throw runtime_error(Errors::Config::DirectiveNotUnique("http", directive));
		throw runtime_error(Errors::Config::UnknownDirective("http", directive));
	}
}

void checkServerDirectives(Directives& directives) {
	map<string, unsigned int> counts;
	for (Directives::iterator kv = directives.begin(); kv != directives.end(); ++kv) {
		const string& directive = kv->first;
		Arguments& arguments = kv->second;
		CHECKFN_SERVER(checkAutoindex);
		CHECKFN_SERVER(checkCgiDir);
		CHECKFN_SERVER(checkCgiExt);
		CHECKFN_SERVER(checkClientMaxBodySize);
		CHECKFN_SERVER(checkErrorPage);
		CHECKFN_SERVER(checkIndex);
		CHECKFN_SERVER(checkListen);
		CHECKFN_SERVER(checkRoot);
		CHECKFN_SERVER(checkServerName);
		CHECKFN_SERVER(checkUploadDir);
		if (counts[directive] > 1)
			throw runtime_error(Errors::Config::DirectiveNotUnique("server", directive));
		throw runtime_error(Errors::Config::UnknownDirective("server", directive));
	}
}

void checkLocationDirectives(Directives& directives) {
	map<string, unsigned int> counts;
	for (Directives::iterator kv = directives.begin(); kv != directives.end(); ++kv) {
		const string& directive = kv->first;
		Arguments& arguments = kv->second;
		CHECKFN_LOCATION(checkAlias);
		CHECKFN_LOCATION(checkAutoindex);
		CHECKFN_LOCATION(checkCgiDir);
		CHECKFN_LOCATION(checkCgiExt);
		CHECKFN_LOCATION(checkClientMaxBodySize);
		CHECKFN_LOCATION(checkErrorPage);
		CHECKFN_LOCATION(checkIndex);
		CHECKFN_LOCATION(checkLimitExcept);
		CHECKFN_LOCATION(checkReturn);
		CHECKFN_LOCATION(checkRoot);
		CHECKFN_LOCATION(checkUploadDir);
		if (counts[directive] > 1)
			throw runtime_error(Errors::Config::DirectiveNotUnique("location", directive));
		throw runtime_error(Errors::Config::UnknownDirective("location", directive));
	}
}

// TODO: @timo: validate that server_name is unique among server with same host:port
// TODO: @timo: validate that host:port is unique among servers
// TODO: @timo: restructure config since some directives can be supplied multiple times (map is not good, rather use multimap)
