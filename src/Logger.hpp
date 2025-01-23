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
	
	static void logexception(const std::exception& exception);
	static void logerror(const std::string& msg);
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
};

void swap(Logger&, Logger&) /* noexcept */;

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#include "Repr.hpp"

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

#define TRACE_DEFAULT_CTOR do { \
		if (Logger::trace()) { \
			std::ostringstream oss; \
			if (Constants::jsonTrace) \
				oss << "{\"event\":\"default constructor\",\"this object\":" << ::repr(*this) << "}\n"; \
			else \
				oss << kwrd(getClass(*this)) + punct("() -> ") << ::repr(*this) << '\n'; \
			cout << oss.str() << std::flush; \
		} \
	} while (false)

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
