#pragma once /* Logger.hpp */

#include <string>
#include <exception>

class Logger {
public:
	~Logger();
	Logger();
	Logger(const Logger& other);
	Logger& operator=(Logger);
	void swap(Logger& other);
	
	static const int TRACE = 0;
	static const int DEBUG = 1;
	static const int INFO  = 2;
	static const int WARN  = 3;
	static const int ERROR = 4;
	static const int FATAL = 5;
	static bool trace();
	static bool debug();
	static bool info();
	static bool warn();
	static bool error();
	static bool fatal();
	static void logTrace(const std::string& msg);
	static void logDebug(const std::string& msg);
	static void logInfo(const std::string& msg);
	static void logWarning(const std::string& msg);
	static void logError(const std::string& msg);
	static void logFatal(const std::string& msg);
};

void swap(Logger&, Logger&) /* noexcept */;

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#include "Repr.hpp"
#include "MacroMagic.h"

using std::cout;

#define TRACE_COPY_ASSIGN_OP do { \
		if (Logger::trace()) { \
			std::ostringstream oss; \
			if (Constants::jsonTrace) \
				oss << "{\"event\":\"copy assignment operator\",\"other object\":" << ::repr(other) << "}\n"; \
			else \
				oss << kwrd(getClass(*this)) + punct("& ") + kwrd(getClass(*this)) + punct("::") + func("operator") + punct("=(") << ::repr(other) << punct(")") + '\n'; \
			cout << oss.str() << std::flush; \
		} \
	} while (false)

#define TRACE_COPY_CTOR do { \
		if (Logger::trace()) { \
			std::ostringstream oss; \
			if (Constants::jsonTrace) \
				oss << "{\"event\":\"copy constructor\",\"other object\":" << ::repr(other) << ",\"this object\":" << ::repr(*this) << "}\n"; \
			else \
				oss << kwrd(getClass(*this)) + punct("(") << ::repr(other) << punct(") -> ") << ::repr(*this) << '\n'; \
			cout << oss.str() << std::flush; \
		} \
	} while (false)

#define GEN_NAMES_FIRST(type, name) \
	<< #type << " " << #name

#define GEN_NAMES(type, name) \
	<< punct(", ") << #type << " " << #name

#define GEN_REPRS_FIRST(_, name) \
	<< (Constants::kwargLogs ? cmt(#name) : "") << (Constants::kwargLogs ? cmt("=") : "") << ::repr(name) 

#define GEN_REPRS(_, name) \
	<< punct(", ") << (Constants::kwargLogs ? cmt(#name) : "") << (Constants::kwargLogs ? cmt("=") : "") << ::repr(name)

#define TRACE_ARG_CTOR(...) do { \
		if (Logger::trace()) { \
			std::ostringstream oss; \
			if (Constants::jsonTrace) \
				IF(IS_EMPTY(__VA_ARGS__)) \
				( \
					oss << "{\"event\":\"default constructor\",\"this object\":" << ::repr(*this) << "}\n"; \
					, \
					oss << "{\"event\":\"(" EXPAND(DEFER(GEN_NAMES_FIRST)(HEAD2(__VA_ARGS__))) FOR_EACH_PAIR(GEN_NAMES, TAIL2(__VA_ARGS__)) << ") constructor\",\"this object\":" << ::repr(*this) << "}\n"; \
				) \
			else \
				IF(IS_EMPTY(__VA_ARGS__)) \
				( \
					oss << kwrd(getClass(*this)) + punct("() -> ") << ::repr(*this) << '\n'; \
					, \
					oss << kwrd(getClass(*this)) + punct("(") EXPAND(DEFER(GEN_REPRS_FIRST)(HEAD2(__VA_ARGS__))) FOR_EACH_PAIR(GEN_REPRS, TAIL2(__VA_ARGS__)) << punct(") -> ") << ::repr(*this) << '\n'; \
				) \
			cout << oss.str() << std::flush; \
		} \
	} while (false)

#define TRACE_DEFAULT_CTOR \
	TRACE_ARG_CTOR()

#define TRACE_DTOR do { \
		if (Logger::trace()) { \
			std::ostringstream oss; \
			if (Constants::jsonTrace) \
				oss << "{\"event\":\"destructor\",\"this object\":" << ::repr(*this) << "}\n"; \
			else \
				oss << punct("~") << ::repr(*this) << '\n'; \
			cout << oss.str() << std::flush; \
		} \
	} while (false)

#define TRACE_SWAP_BEGIN do { \
		if (Logger::trace()) { \
			std::ostringstream oss; \
			if (Constants::jsonTrace) { \
				oss << "{\"event\":\"object swap\",\"this object\":" << ::repr(*this) << ",\"other object\":" << ::repr(other) << "}\n"; \
			} else { \
				oss << cmt("<Swapping " + std::string(getClass(*this)) + " *this:") + '\n'; \
				oss << ::repr(*this) << '\n'; \
				oss << cmt("with the following" + std::string(getClass(*this)) + "object:") + '\n'; \
				oss << ::repr(other) << '\n'; \
			} \
			cout << oss.str() << std::flush; \
		} \
	} while (false)

#define TRACE_SWAP_END do { \
		if (Logger::trace()) { \
			std::ostringstream oss; \
			if (!Constants::jsonTrace) \
				oss << cmt(std::string(getClass(*this)) + " swap done>") + '\n'; \
			cout << oss.str() << std::flush; \
		} \
	} while (false)
