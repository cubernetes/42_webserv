#include <iostream> /* std::cout, std::swap, std::ostream */
#include <string> /* std::string */
#include <sstream> /* std::stringstream */

#include "repr.hpp"
#include "Logger.hpp"
#include "Constants.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::swap;
using std::string;
using std::ostream;
using std::stringstream;

// De- & Constructors
Logger::~Logger() {
	// TRACE_DTOR;
	if (Logger::trace()) print_dtor(::repr(*this));
}

Logger::Logger() : _id(_idCntr++) {
	reflect(); // TRACE_DEFAULT_CTOR;
	if (Logger::trace()) print_default_ctor(::repr(*this));
}

Logger::Logger(const Logger& other) : _id(_idCntr++) {
	reflect(); // TRACE_COPY_CTOR;
	if (Logger::trace()) print_copy_ctor(::repr(other), ::repr(*this));
}

// Copy-assignment operator (using copy-swap idiom)
Logger& Logger::operator=(Logger other) /* noexcept */ {
	// TRACE_COPY_ASSIGN_OP;
	if (Logger::trace()) print_copy_assign_op(::repr(other));
	::swap(*this, other);
	return *this;
}

void Logger::swap(Logger& other) /* noexcept */ {
	// TRACE_SWAP_BEGIN;
	if (Logger::trace()) print_swap_begin(::repr(*this), ::repr(other));
	::swap(_id, other._id);
	// TRACE_SWAP_END;
	if (Logger::trace()) print_swap_end();
}

Logger::operator string() const { return ::repr(*this); }

void swap(Logger& a, Logger& b) /* noexcept */ { a.swap(b); }
ostream& operator<<(ostream& os, const Logger& other) { return os << static_cast<string>(other); }

// Keeping track of the instances
unsigned int Logger::_idCntr = 0;

void Logger::logexception(const std::exception& exception) {
	cerr << exception.what() << endl;
}

void Logger::logerror(const char* error) {
	cerr << error << endl;
}

bool Logger::trace() {
	return Constants::logLevel <= logLevelTrace;
}

bool Logger::debug() {
	return Constants::logLevel <= logLevelDebug;
}

bool Logger::info() {
	return Constants::logLevel <= logLevelInfo;
}

bool Logger::warn() {
	return Constants::logLevel <= logLevelWarn;
}

bool Logger::error() {
	return Constants::logLevel <= logLevelError;
}

bool Logger::fatal() {
	return Constants::logLevel <= logLevelFatal;
}
