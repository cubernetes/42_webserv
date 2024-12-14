// <GENERATED>
#include <iostream> /* std::cout, std::swap, std::ostream */
#include <string> /* std::string */
#include <sstream> /* std::stringstream */

#include "repr.hpp"
#include "CgiHandler.hpp"
#include "Logger.hpp"

using std::cout;
using std::swap;
using std::string;
using std::ostream;
using std::stringstream;

// De- & Constructors
CgiHandler::~CgiHandler() {
	if (Logger::trace())
		cout << ANSI_PUNCT "~" << *this << '\n';
}

CgiHandler::CgiHandler() : _extension(), _program(), _id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "CgiHandler" ANSI_PUNCT "() -> " << *this << '\n';
}

CgiHandler::CgiHandler(const string& extension) : _extension(extension), _program(), _id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "CgiHandler" ANSI_PUNCT "(" << ::repr(extension) << ANSI_PUNCT ") -> " << *this << '\n';
}

CgiHandler::CgiHandler(const char* extension) : _extension(extension), _program(), _id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "CgiHandler" ANSI_PUNCT "(" << ::repr(extension) << ANSI_PUNCT ") -> " << *this << '\n';
}

CgiHandler::CgiHandler(const string& extension, const string& program) : _extension(extension), _program(program), _id(_idCntr++) {
	if (Logger::trace())
		cout << *this << ANSI_PUNCT " -> " << *this << '\n';
}

CgiHandler::CgiHandler(const CgiHandler& other) : _extension(other._extension), _program(other._program), _id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "CgiHandler" ANSI_PUNCT "(" << ::repr(other) << ANSI_PUNCT ") -> " << *this << '\n';
}

// Copy-assignment operator (using copy-swap idiom)
CgiHandler& CgiHandler::operator=(CgiHandler other) /* noexcept */ {
	if (Logger::trace())
		cout << ANSI_KWRD "CgiHandler" ANSI_PUNCT "& " ANSI_KWRD "CgiHandler" ANSI_PUNCT "::" ANSI_FUNC "operator" ANSI_PUNCT "=(" << ::repr(other) << ANSI_PUNCT ")" ANSI_RST "\n";
	::swap(*this, other);
	return *this;
}

// Generated getters
const string& CgiHandler::get_extension() const { return _extension; }
const string& CgiHandler::get_program() const { return _program; }

// Generated setters
void CgiHandler::set_extension(const string& value) { _extension = value; }
void CgiHandler::set_program(const string& value) { _program = value; }

// Generated member functions
string CgiHandler::repr() const {
	stringstream out;
	out << ANSI_KWRD "CgiHandler" ANSI_PUNCT "(" << ::repr(_extension) << ANSI_PUNCT ", " << ::repr(_program) << ANSI_PUNCT ", " << ::repr(_id) << ANSI_PUNCT ")" ANSI_RST;
	return out.str();
}

void CgiHandler::swap(CgiHandler& other) /* noexcept */ {
	if (Logger::trace()) {
		cout << ANSI_CMT "<Swapping CgiHandler *this:" ANSI_RST "\n";
		cout << *this << '\n';
		cout << ANSI_CMT "with the following CgiHandler object:" ANSI_RST "\n";
		cout << other << '\n';
	}
	::swap(_extension, other._extension);
	::swap(_program, other._program);
	::swap(_id, other._id);
	if (Logger::trace())
		cout << ANSI_CMT "CgiHandler swap done>" ANSI_RST "\n";
}

CgiHandler::operator string() const { return ::repr(*this); }

// Generated free functions
void swap(CgiHandler& a, CgiHandler& b) /* noexcept */ { a.swap(b); }
ostream& operator<<(ostream& os, const CgiHandler& other) { return os << static_cast<string>(other); }

// Keeping track of the instances
unsigned int CgiHandler::_idCntr = 0;
// </GENERATED>
