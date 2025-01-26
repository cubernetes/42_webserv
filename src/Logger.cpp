#include <iostream>

#include "Logger.hpp"
#include "Constants.hpp"

using std::cerr;
using std::endl;
using std::swap;

// De- & Constructors
Logger::~Logger() { }
Logger::Logger() { }
Logger::Logger(const Logger& other) { (void)other; }

// Copy-assignment operator (using copy-swap idiom)
Logger& Logger::operator=(Logger other) /* noexcept */ {
	::swap(*this, other);
	return *this;
}

void Logger::swap(Logger& other) /* noexcept */ {
	(void)other;
}

void swap(Logger& a, Logger& b) /* noexcept */ { a.swap(b); }
// end of boilerplate

// TODO: @timo: improve logging
void Logger::logTrace(const std::string& msg) {
	if (Logger::trace())
		cerr << msg << endl;
}

void Logger::logDebug(const std::string& msg) {
	if (Logger::debug())
	cerr << msg << endl;
}

void Logger::logInfo(const std::string& msg) {
	if (Logger::info())
	cerr << msg << endl;
}

void Logger::logWarning(const std::string& msg) {
	if (Logger::warn())
		cerr << msg << endl;
}

void Logger::logError(const std::string& msg) {
	if (Logger::error())
		cerr << msg << endl;
}

void Logger::logFatal(const std::string& msg) {
	if (Logger::fatal())
		cerr << msg << endl;
}

bool Logger::trace() {
	return Constants::logLevel <= TRACE;
}

bool Logger::debug() {
	return Constants::logLevel <= DEBUG;
}

bool Logger::info() {
	return Constants::logLevel <= INFO;
}

bool Logger::warn() {
	return Constants::logLevel <= WARN;
}

bool Logger::error() {
	return Constants::logLevel <= ERROR;
}

bool Logger::fatal() {
	return Constants::logLevel <= FATAL;
}
