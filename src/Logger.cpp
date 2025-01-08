// <GENERATED>
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
	if (Logger::trace())
		cout << punct("~") << *this << '\n';
}

Logger::Logger() : _id(_idCntr++) {
	if (Logger::trace())
		cout << kwrd("Logger") + punct("() -> ") << *this << '\n';
}

Logger::Logger(const Logger& other) : _id(_idCntr++) {
	if (Logger::trace())
		cout << kwrd("Logger") + punct("(") << ::repr(other) << punct(") -> ") << *this << '\n';
}

// Copy-assignment operator (using copy-swap idiom)
Logger& Logger::operator=(Logger other) /* noexcept */ {
	if (Logger::trace())
		cout << kwrd("Logger") + punct("& ") + kwrd("Logger") + punct("::") + func("operator") + punct("=(") << ::repr(other) << punct(")") + "\n";
	::swap(*this, other);
	return *this;
}

// Generated member functions
string Logger::repr() const {
	stringstream out;
	out << kwrd("Logger") + punct("(") << ::repr(_id) << punct(")");
	return out.str();
}

void Logger::swap(Logger& other) /* noexcept */ {
	if (Logger::trace()) {
		cout << cmt("<Swapping Logger *this:") + "\n";
		cout << *this << '\n';
		cout << cmt("with the following Logger object:") + "\n";
		cout << other << '\n';
	}
	::swap(_id, other._id);
	if (Logger::trace())
		cout << cmt("Logger swap done>") + "\n";
}

Logger::operator string() const { return ::repr(*this); }

// Generated free functions
void swap(Logger& a, Logger& b) /* noexcept */ { a.swap(b); }
ostream& operator<<(ostream& os, const Logger& other) { return os << static_cast<string>(other); }

// Keeping track of the instances
unsigned int Logger::_idCntr = 0;
// </GENERATED>

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
