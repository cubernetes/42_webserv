#pragma once

#include <exception>
#include <string>
#include <iostream>

using std::string;

// Logger is used in repr, therefore, the Logger MUST not use Reflection
class Logger {
public:
	~Logger();
	Logger();
	Logger(const Logger& other);
	Logger& operator=(Logger);
	void swap(Logger& other);
	
	static void logexception(const std::exception& exception);
	static void logerror(const char* error);
	static const int logLevelTrace = 0;
	static const int logLevelDebug = 1;
	static const int logLevelInfo  = 2;
	static const int logLevelWarn  = 3;
	static const int logLevelError = 4;
	static const int logLevelFatal = 5;
	static bool trace();
	static bool debug();
	static bool info();
	static bool warn();
	static bool error();
	static bool fatal();
};

void swap(Logger&, Logger&) /* noexcept */;

#include "repr.hpp"

#define TRACE_COPY_ASSIGN_OP do { \
	if (Constants::jsonTrace) \
		cout << "{\"event\":\"copy assignment operator\",\"other object\":" << ::repr(other) << "}\n"; \
	else \
		cout << kwrd(get_class(*this)) + punct("& ") + kwrd(get_class(*this)) + punct("::") + func("operator") + punct("=(") << ::repr(other) << punct(")") + '\n'; \
	} while (false)

#define TRACE_COPY_CTOR do { \
	if (Constants::jsonTrace) \
		cout << "{\"event\":\"copy constructor\",\"other object\":" << ::repr(other) << ",\"this object\":" << ::repr(*this) << "}\n"; \
	else \
		cout << kwrd(get_class(*this)) + punct("(") << ::repr(other) << punct(") -> ") << ::repr(*this) << '\n'; \
	} while (false)

#define TRACE_DEFAULT_CTOR do { \
	if (Constants::jsonTrace) \
		cout << "{\"event\":\"default constructor\",\"this object\":" << ::repr(*this) << "}\n"; \
	else \
		cout << kwrd(get_class(*this)) + punct("() -> ") << ::repr(*this) << '\n'; \
	} while (false)

#define TRACE_DTOR do { \
	if (Constants::jsonTrace) \
		cout << "{\"event\":\"destructor\",\"this object\":" << ::repr(*this) << "}\n"; \
	else \
		cout << punct("~") << ::repr(*this) << '\n'; \
	} while (false)

#define TRACE_SWAP_BEGIN do { \
	cout << cmt("<Swapping " + string(get_class(*this)) + " *this:") + '\n'; \
	cout << ::repr(*this) << '\n'; \
	cout << cmt("with the following" + string(get_class(*this)) + "object:") + '\n'; \
	cout << ::repr(other) << '\n'; \
	} while (false)

#define TRACE_SWAP_END do { \
	cout << cmt(string(get_class(*this)) + " swap done>") + '\n'; \
	} while (false)
