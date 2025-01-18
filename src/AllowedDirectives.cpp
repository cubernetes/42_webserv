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

using std::map;
using std::vector;
using std::pair;
using std::string;
using std::runtime_error;

// max < 0 -> no maximum
static inline void ensureArity(const Arguments& arguments, int min, int max) {
	if (static_cast<int>(arguments.size()) >= min && (static_cast<int>(arguments.size()) <= max || max < 0))
		return; // all good
	throw runtime_error(Errors::Config::InvalidDirectiveArgumentCount);
}

static inline void ensureOneOfStrings(const string& argument, const Arguments& possibilities) {
	if (std::find(possibilities.begin(), possibilities.end(), argument) == possibilities.end()) {
		throw runtime_error(Errors::Config::InvalidDirectiveArgument);
	}
}

static inline void ensureNotEmpty(const string& argument) {
	if (argument.empty())
		throw runtime_error(Errors::Config::DirectiveArgumentEmpty);
}

static inline void ensureInt(const string& argument) {
	ensureNotEmpty(argument);
	string::const_iterator c = argument.begin();
	while (c != argument.end() && std::isdigit(*c)) ++c;
	if (c == argument.end())
		return;
	throw runtime_error(Errors::Config::DirectiveArgumentNotNumeric);
}

static inline void ensureValidSize(const string& argument) {
	ensureNotEmpty(argument);
	string::const_iterator c = argument.begin();
	while (c != argument.end() && std::isdigit(*c)) ++c;
	if (c == argument.end())
		return;
	if (std::strchr("KMGT", std::toupper(*c)) && c + 1 == argument.end())
		return;
	throw runtime_error(Errors::Config::DirectiveInvalidSizeArgument);
}

static inline void ensureOnOff(const string& argument) {
	if (argument == "on" || argument == "yes" || argument == "true")
		return;
	if (argument == "off" || argument == "no" || argument == "false")
		return;
	throw runtime_error(Errors::Config::DirectiveInvalidBooleanArgument);
}

// only allow official status codes, might even prune this list even further
// https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
static inline void ensureStatusCode(const string& argument) {
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
	throw runtime_error(Errors::Config::DirectiveInvalidStatusCodeArgument);
}

static inline void ensureValidHost(const string& argument) {
	if (argument == "*")
		return;
	char buf[16];
	if (inet_pton(AF_INET, argument.c_str(), &buf) == 1)
		return;
	if (inet_pton(AF_INET6, argument.c_str(), &buf) == 1)
		throw runtime_error(Errors::Config::DirectiveIPv6NotSupported);
	throw runtime_error(Errors::Config::DirectiveInvalidIpAddressArgument);
}

static inline void ensureValidPort(const string& argument) {
	char *end;
	long l;

	errno = 0;
	l = strtol(argument.c_str(), &end, 10);
	if ((errno == ERANGE && l == LONG_MAX) || l > Constants::highestPort)
		throw runtime_error(Errors::Config::DirectiveArgumentPortNumberTooHigh);
	if ((errno == ERANGE && l == LONG_MIN) || l < 1)
		throw runtime_error(Errors::Config::DirectiveArgumentPortNumberTooLow);
	if (argument.empty() || *end != '\0')
		throw runtime_error(Errors::Config::DirectiveArgumentInvalidPort);
}

static inline void ensureGloballyUnique(const string& argument) {
	static map<string, int> counts;
	++counts[argument];
	if (counts[argument] > 1) {
		throw runtime_error(Errors::Config::DirectiveArgumentNotUnique);
	}
}


static inline bool checkPid(const string& key, const Arguments& arguments) {
	if (key == "pid") {
		ensureArity(arguments, 1, 1);
		ensureNotEmpty(arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkWorkerProcesses(const string& key, const Arguments& arguments) {
	if (key == "worker_processes") {
		ensureArity(arguments, 1, 1);
		ensureNotEmpty(arguments[0]);
		if (arguments[0] != "auto")
			ensureInt(arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkIndex(const string& key, const Arguments& arguments) {
	if (key == "index") {
		ensureArity(arguments, 1, -1);
		for (Arguments::const_iterator argument = arguments.begin(); argument != arguments.end(); ++argument) {
			ensureNotEmpty(*argument);
		}
		return true;
	}
	return false;
}

static inline bool checkRoot(const string& key, const Arguments& arguments) {
	if (key == "root") {
		ensureArity(arguments, 1, 1);
		ensureNotEmpty(arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkAccessLog(const string& key, const Arguments& arguments) {
	if (key == "access_log") {
		ensureArity(arguments, 1, 1);
		ensureNotEmpty(arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkErrorLog(const string& key, const Arguments& arguments) {
	if (key == "error_log") {
		ensureArity(arguments, 1, 1);
		ensureNotEmpty(arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkListen(const string& key, const Arguments& arguments) {
	if (key == "listen") {
		ensureArity(arguments, 2, 2);
		ensureValidHost(arguments[0]);
		ensureValidPort(arguments[1]);
		return true;
	}
	return false;
}

static inline bool checkServerName(const string& key, const Arguments& arguments) {
	if (key == "server_name") {
		ensureArity(arguments, 1, 1);
		ensureGloballyUnique(arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkClientMaxBodySize(const string& key, const Arguments& arguments) {
	if (key == "client_max_body_size") {
		ensureArity(arguments, 1, 1);
		ensureValidSize(arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkAllowMethods(const string& key, const Arguments& arguments) {
	if (key == "allowed_methods") {
		ensureArity(arguments, 1, -1);
		for (Arguments::const_iterator argument = arguments.begin(); argument != arguments.end(); ++argument) {
			ensureOneOfStrings(*argument, VEC(string, "GET", "POST", "DELETE"));
		}
		return true;
	}
	return false;
}

static inline bool checkAlias(const string& key, const Arguments& arguments) {
	if (key == "alias") {
		ensureArity(arguments, 1, 1);
		ensureNotEmpty(arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkReturn(const string& key, const Arguments& arguments) {
	if (key == "return") {
		ensureArity(arguments, 2, 2);
		ensureStatusCode(arguments[0]);
		ensureNotEmpty(arguments[1]);
		return true;
	}
	return false;
}

static inline bool checkAutoindex(const string& key, const Arguments& arguments) {
	if (key == "autoindex") {
		ensureArity(arguments, 1, 1);
		ensureOnOff(arguments[0]);
		return true;
	}
	return false;
}

void checkGlobalDirectives(const Directives& directives) {
	for (Directives::const_iterator kv = directives.begin(); kv != directives.end(); ++kv) {
		const string& key = kv->first;
		const Arguments& arguments = kv->second;
		if (checkPid(key, arguments)) continue;
		if (checkWorkerProcesses(key, arguments)) continue;
		if (checkIndex(key, arguments)) continue;
		if (checkRoot(key, arguments)) continue;
		if (checkAccessLog(key, arguments)) continue;
		if (checkErrorLog(key, arguments)) continue;
		if (checkListen(key, arguments)) continue;
		throw runtime_error(Errors::Config::UnknownDirective);
	}
}

void checkServerDirectives(const Directives& directives) {
	for (Directives::const_iterator kv = directives.begin(); kv != directives.end(); ++kv) {
		const string& key = kv->first;
		const Arguments& arguments = kv->second;
		if (checkServerName(key, arguments)) continue;
		if (checkIndex(key, arguments)) continue;
		if (checkRoot(key, arguments)) continue;
		if (checkAccessLog(key, arguments)) continue;
		if (checkErrorLog(key, arguments)) continue;
		if (checkListen(key, arguments)) continue;
		if (checkClientMaxBodySize(key, arguments)) continue;
		throw runtime_error(Errors::Config::UnknownDirective);
	}
}

void checkRouteDirectives(const Directives& directives) {
	for (Directives::const_iterator kv = directives.begin(); kv != directives.end(); ++kv) {
		const string& key = kv->first;
		const Arguments& arguments = kv->second;
		if (checkAllowMethods(key, arguments)) continue;
		if (checkIndex(key, arguments)) continue;
		if (checkRoot(key, arguments)) continue;
		if (checkAlias(key, arguments)) continue;
		if (checkReturn(key, arguments)) continue;
		if (checkAutoindex(key, arguments)) continue;
		throw runtime_error(Errors::Config::UnknownDirective);
	}
}
