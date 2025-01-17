#include <iostream> /* std::cout, std::swap, std::ostream */
#include <string> /* std::string */
#include <sstream> /* std::stringstream */

#include "Logger.hpp"
#include "CgiHandler.hpp"

using std::cout;
using std::swap;
using std::string;
using std::ostream;
using std::stringstream;

// De- & Constructors
CgiHandler::~CgiHandler() {
	TRACE_DTOR;
}

CgiHandler::CgiHandler() : _extension(), _program() {
	reflect(); TRACE_DEFAULT_CTOR;
}

CgiHandler::CgiHandler(const string& extension) : _extension(extension), _program() {
	reflect(); // TRACE_DEFAULT_CTOR;
}

CgiHandler::CgiHandler(const char* extension) : _extension(extension), _program() {
	reflect(); // TRACE_DEFAULT_CTOR;
}

CgiHandler::CgiHandler(const string& extension, const string& program) : _extension(extension), _program(program) {
	reflect(); // TRACE_DEFAULT_CTOR;
}

CgiHandler::CgiHandler(const CgiHandler& other) : _extension(other._extension), _program(other._program) {
	reflect(); TRACE_COPY_CTOR;
}

// Copy-assignment operator (using copy-swap idiom)
CgiHandler& CgiHandler::operator=(CgiHandler other) /* noexcept */ {
	TRACE_COPY_ASSIGN_OP;
	::swap(*this, other);
	return *this;
}

// Generated getters
const string& CgiHandler::get_extension() const { return _extension; }
const string& CgiHandler::get_program() const { return _program; }

void CgiHandler::swap(CgiHandler& other) /* noexcept */ {
	TRACE_SWAP_BEGIN;
	::swap(_extension, other._extension);
	::swap(_program, other._program);
	TRACE_SWAP_END;
}

CgiHandler::operator string() const { return ::repr(*this); }

// Generated free functions
void swap(CgiHandler& a, CgiHandler& b) /* noexcept */ { a.swap(b); }
ostream& operator<<(ostream& os, const CgiHandler& other) { return os << static_cast<string>(other); }
