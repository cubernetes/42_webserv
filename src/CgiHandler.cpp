#include <ostream>
#include <string>

#include "CgiHandler.hpp"
#include "Logger.hpp"

using std::swap;
using std::string;

// De- & Constructors
CgiHandler::~CgiHandler() {
	TRACE_DTOR;
}

CgiHandler::CgiHandler() :
	_extension(),
	_program() {
	TRACE_DEFAULT_CTOR;
}

CgiHandler::CgiHandler(const CgiHandler& other) :
	_extension(other._extension),
	_program(other._program) {
	TRACE_COPY_CTOR;
}

// copy swap idiom
CgiHandler& CgiHandler::operator=(CgiHandler other) /* noexcept */ {
	TRACE_COPY_ASSIGN_OP;
	::swap(*this, other);
	return *this;
}

// Getters
const string& CgiHandler::get_extension() const { return _extension; }
const string& CgiHandler::get_program() const { return _program; }

void CgiHandler::swap(CgiHandler& other) /* noexcept */ {
	TRACE_SWAP_BEGIN;
	::swap(_extension, other._extension);
	::swap(_program, other._program);
	TRACE_SWAP_END;
}

CgiHandler::operator string() const { return ::repr(*this); }

void swap(CgiHandler& a, CgiHandler& b) /* noexcept */ {
	a.swap(b);
}

std::ostream& operator<<(std::ostream& os, const CgiHandler& other) {
	return os << static_cast<string>(other);
}
