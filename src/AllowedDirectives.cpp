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
static inline void ensureArity(const string& directive, const Arguments& arguments, int min, int max) {
	int ac = static_cast<int>(arguments.size());
	if (ac >= min && (ac <= max || max < 0))
		return; // all good
	throw runtime_error(Errors::Config::InvalidDirectiveArgumentCount(directive, ac, min, max));
}

static inline void ensureOneOfStrings(const string& directive, const string& argument, const Arguments& possibilities) {
	if (std::find(possibilities.begin(), possibilities.end(), argument) == possibilities.end()) {
		throw runtime_error(Errors::Config::InvalidDirectiveArgument(directive, argument, possibilities));
	}
}

static inline void ensureNotEmpty(const string& directive, const string& argument) {
	if (argument.empty())
		throw runtime_error(Errors::Config::DirectiveArgumentEmpty(directive));
}

static inline void ensureInt(const string& directive, const string& argument) {
	ensureNotEmpty(directive, argument);
	string::const_iterator c = argument.begin();
	while (c != argument.end() && std::isdigit(*c)) ++c;
	if (c == argument.end())
		return;
	throw runtime_error(Errors::Config::DirectiveArgumentNotNumeric(directive, argument));
}

static inline void ensureValidSize(const string& directive, const string& argument) {
	ensureNotEmpty(directive, argument);
	string::const_iterator c = argument.begin();
	while (c != argument.end() && std::isdigit(*c)) ++c;
	if (c == argument.end())
		return;
	if (std::strchr("KMGT", std::toupper(*c)) && c + 1 == argument.end())
		return;
	throw runtime_error(Errors::Config::DirectiveInvalidSizeArgument(directive, argument));
}

static inline void ensureOnOff(const string& directive, const string& argument) {
	if (argument == "on" || argument == "yes" || argument == "true")
		return;
	if (argument == "off" || argument == "no" || argument == "false")
		return;
	throw runtime_error(Errors::Config::DirectiveInvalidBooleanArgument(directive, argument));
}

// only allow official status codes, might even prune this list even further
// https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
static inline void ensureStatusCode(const string& directive, const string& argument) {
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
	throw runtime_error(Errors::Config::DirectiveInvalidStatusCodeArgument(directive, argument));
}

static inline void ensureValidHost(const string& directive, const string& argument) {
	if (argument == "*")
		return;
	char buf[16];
	if (inet_pton(AF_INET, argument.c_str(), &buf) == 1)
		return;
	if (inet_pton(AF_INET6, argument.c_str(), &buf) == 1)
		throw runtime_error(Errors::Config::DirectiveIPv6NotSupported(directive));
	throw runtime_error(Errors::Config::DirectiveInvalidIpAddressArgument(directive, argument));
}

static inline void ensureValidPort(const string& directive, const string& argument) {
	char *end;
	long l;

	errno = 0;
	l = strtol(argument.c_str(), &end, 10);
	if ((errno == ERANGE && l == LONG_MAX) || l > Constants::highestPort)
		throw runtime_error(Errors::Config::DirectiveArgumentPortNumberTooHigh(directive, argument));
	if ((errno == ERANGE && l == LONG_MIN) || l < 1)
		throw runtime_error(Errors::Config::DirectiveArgumentPortNumberTooLow(directive, argument));
	if (argument.empty() || *end != '\0')
		throw runtime_error(Errors::Config::DirectiveArgumentInvalidPort(directive, argument));
}

static inline void ensureGloballyUnique(const string& directive, const string& argument) {
	static map<string, int> counts;
	++counts[argument];
	if (counts[argument] > 1) {
		throw runtime_error(Errors::Config::DirectiveArgumentNotUnique(directive, argument));
	}
}


static inline bool checkPid(const string& directive, const Arguments& arguments) {
	if (directive == "pid") {
		ensureArity(directive, arguments, 1, 1);
		ensureNotEmpty(directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkWorkerProcesses(const string& directive, const Arguments& arguments) {
	if (directive == "worker_processes") {
		ensureArity(directive, arguments, 1, 1);
		ensureNotEmpty(directive, arguments[0]);
		if (arguments[0] != "auto")
			ensureInt(directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkIndex(const string& directive, const Arguments& arguments) {
	if (directive == "index") {
		ensureArity(directive, arguments, 1, -1);
		for (Arguments::const_iterator argument = arguments.begin(); argument != arguments.end(); ++argument) {
			ensureNotEmpty(directive, *argument);
		}
		return true;
	}
	return false;
}

static inline bool checkRoot(const string& directive, const Arguments& arguments) {
	if (directive == "root") {
		ensureArity(directive, arguments, 1, 1);
		ensureNotEmpty(directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkAccessLog(const string& directive, const Arguments& arguments) {
	if (directive == "access_log") {
		ensureArity(directive, arguments, 1, 1);
		ensureNotEmpty(directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkErrorLog(const string& directive, const Arguments& arguments) {
	if (directive == "error_log") {
		ensureArity(directive, arguments, 1, 1);
		ensureNotEmpty(directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkListen(const string& directive, const Arguments& arguments) {
	if (directive == "listen") {
		ensureArity(directive, arguments, 2, 2);
		ensureValidHost(directive, arguments[0]);
		ensureValidPort(directive, arguments[1]);
		return true;
	}
	return false;
}

static inline bool checkServerName(const string& directive, const Arguments& arguments) {
	if (directive == "server_name") {
		ensureArity(directive, arguments, 1, 1);
		ensureGloballyUnique(directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkClientMaxBodySize(const string& directive, const Arguments& arguments) {
	if (directive == "client_max_body_size") {
		ensureArity(directive, arguments, 1, 1);
		ensureValidSize(directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkAllowMethods(const string& directive, const Arguments& arguments) {
	if (directive == "allowed_methods") {
		ensureArity(directive, arguments, 1, -1);
		for (Arguments::const_iterator argument = arguments.begin(); argument != arguments.end(); ++argument) {
			ensureOneOfStrings(directive, *argument, VEC(string, "GET", "POST", "DELETE"));
		}
		return true;
	}
	return false;
}

static inline bool checkAlias(const string& directive, const Arguments& arguments) {
	if (directive == "alias") {
		ensureArity(directive, arguments, 1, 1);
		ensureNotEmpty(directive, arguments[0]);
		return true;
	}
	return false;
}

static inline bool checkReturn(const string& directive, const Arguments& arguments) {
	if (directive == "return") {
		ensureArity(directive, arguments, 2, 2);
		ensureStatusCode(directive, arguments[0]);
		ensureNotEmpty(directive, arguments[1]);
		return true;
	}
	return false;
}

static inline bool checkAutoindex(const string& directive, const Arguments& arguments) {
	if (directive == "autoindex") {
		ensureArity(directive, arguments, 1, 1);
		ensureOnOff(directive, arguments[0]);
		return true;
	}
	return false;
}

void checkGlobalDirectives(const Directives& directives) {
	for (Directives::const_iterator kv = directives.begin(); kv != directives.end(); ++kv) {
		const string& directive = kv->first;
		const Arguments& arguments = kv->second;
		if (checkPid(directive, arguments)) continue;
		if (checkWorkerProcesses(directive, arguments)) continue;
		if (checkIndex(directive, arguments)) continue;
		if (checkRoot(directive, arguments)) continue;
		if (checkAccessLog(directive, arguments)) continue;
		if (checkErrorLog(directive, arguments)) continue;
		if (checkListen(directive, arguments)) continue;
		throw runtime_error(Errors::Config::UnknownDirective(directive));
	}
}

void checkServerDirectives(const Directives& directives) {
	for (Directives::const_iterator kv = directives.begin(); kv != directives.end(); ++kv) {
		const string& directive = kv->first;
		const Arguments& arguments = kv->second;
		if (checkServerName(directive, arguments)) continue;
		if (checkIndex(directive, arguments)) continue;
		if (checkRoot(directive, arguments)) continue;
		if (checkAccessLog(directive, arguments)) continue;
		if (checkErrorLog(directive, arguments)) continue;
		if (checkListen(directive, arguments)) continue;
		if (checkClientMaxBodySize(directive, arguments)) continue;
		throw runtime_error(Errors::Config::UnknownDirective(directive));
	}
}

void checkRouteDirectives(const Directives& directives) {
	for (Directives::const_iterator kv = directives.begin(); kv != directives.end(); ++kv) {
		const string& directive = kv->first;
		const Arguments& arguments = kv->second;
		if (checkAllowMethods(directive, arguments)) continue;
		if (checkIndex(directive, arguments)) continue;
		if (checkRoot(directive, arguments)) continue;
		if (checkAlias(directive, arguments)) continue;
		if (checkReturn(directive, arguments)) continue;
		if (checkAutoindex(directive, arguments)) continue;
		throw runtime_error(Errors::Config::UnknownDirective(directive));
	}
}
