#include <iostream> /* std::cout, std::swap, std::ostream */
#include <string> /* std::string */
#include <sstream> /* std::stringstream */

// #include "repr.hpp"
#include "CgiHandler.hpp"
#include "Logger.hpp"

using std::cout;
using std::swap;
using std::string;
using std::ostream;
using std::stringstream;

// De- & Constructors
CgiHandler::~CgiHandler() {
}

CgiHandler::CgiHandler() : _extension(), _program(), _id(_idCntr++) {
}

CgiHandler::CgiHandler(const string& extension) : _extension(extension), _program(), _id(_idCntr++) {
}

CgiHandler::CgiHandler(const char* extension) : _extension(extension), _program(), _id(_idCntr++) {
}

CgiHandler::CgiHandler(const string& extension, const string& program) : _extension(extension), _program(program), _id(_idCntr++) {
}

CgiHandler::CgiHandler(const CgiHandler& other) : _extension(other._extension), _program(other._program), _id(_idCntr++) {
}

// Copy-assignment operator (using copy-swap idiom)
CgiHandler& CgiHandler::operator=(CgiHandler other) /* noexcept */ {
	::swap(*this, other);
	return *this;
}

// Generated getters
const string& CgiHandler::get_extension() const { return _extension; }
const string& CgiHandler::get_program() const { return _program; }

// Generated setters
void CgiHandler::set_extension(const string& value) { _extension = value; }
void CgiHandler::set_program(const string& value) { _program = value; }

void CgiHandler::swap(CgiHandler& other) /* noexcept */ {
	::swap(_extension, other._extension);
	::swap(_program, other._program);
	::swap(_id, other._id);
}

CgiHandler::operator string() const { return ::repr(*this); }

// Generated free functions
void swap(CgiHandler& a, CgiHandler& b) /* noexcept */ { a.swap(b); }
ostream& operator<<(ostream& os, const CgiHandler& other) { return os << static_cast<string>(other); }

// Keeping track of the instances
unsigned int CgiHandler::_idCntr = 0;
