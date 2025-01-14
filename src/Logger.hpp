#pragma once

#include <exception>
#include <string>
#include <iostream>

using std::string;

#include "Reflection.hpp"

#define TRACE_DTOR if (Logger::trace()) print_dtor(::repr(*this))
#define TRACE_DEFAULT_CTOR if (Logger::trace()) print_default_ctor(::repr(*this))
#define TRACE_COPY_CTOR if (Logger::trace()) print_copy_ctor(::repr(other), ::repr(*this))
#define TRACE_COPY_ASSIGN_OP if (Logger::trace()) print_copy_assign_op(::repr(other))
#define TRACE_SWAP_BEGIN if (Logger::trace()) print_swap_begin(::repr(*this), ::repr(other))
#define TRACE_SWAP_END if (Logger::trace()) print_swap_end()

class Logger /* : public Reflection */ {
public:
	~Logger();
	Logger();
	Logger(const Logger& other);
	Logger& operator=(Logger);
	void swap(Logger& other);
	operator string() const;
	
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
private:
	// REFLECT(
	// 	"Logger",
	// 	DECL(unsigned int, _id)
	// )
	unsigned int _id;

	static unsigned int _idCntr;
};

void swap(Logger&, Logger&) /* noexcept */;
std::ostream& operator<<(std::ostream&, const Logger&);
