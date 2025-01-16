#include <iostream> /* std::cout, std::swap, std::ostream */
#include <string> /* std::string */
#include <sstream> /* std::stringstream */

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
Logger::~Logger() { }
Logger::Logger() : _id(_idCntr++) { }
Logger::Logger(const Logger& other) : _id(_idCntr++) { (void)other; }

// Copy-assignment operator (using copy-swap idiom)
Logger& Logger::operator=(Logger other) /* noexcept */ {
	::swap(*this, other);
	return *this;
}

void Logger::swap(Logger& other) /* noexcept */ {
	::swap(_id, other._id);
}

void swap(Logger& a, Logger& b) /* noexcept */ { a.swap(b); }

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
