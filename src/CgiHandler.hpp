#pragma once /* CgiHandler.hpp */

#include <string> /* std::string */
#include <iostream> /* std::ostream */

#include "Reflection.hpp"

using std::string;
using std::ostream;

class CgiHandler : public Reflection {
public:
		~CgiHandler();
		CgiHandler();
		explicit CgiHandler(const string&); // TODO: do we need?
		explicit CgiHandler(const char*); // TODO: do we need?
		CgiHandler(const string& extension, const string& program);
		CgiHandler(const CgiHandler& other);
		CgiHandler& operator=(CgiHandler);
		void swap(CgiHandler&);
		operator string() const;

		const string& get_extension() const; // TODO: can't we just make them public?
		const string& get_program() const; // TODO: can't we just make them public?

		void set_extension(const string&); // TODO: can't we just make them public?
		void set_program(const string&); // TODO: can't we just make them public?
private:
	REFLECT(
		CgiHandler,
		DECL(string, _extension),
		DECL(string, _program),
		DECL(unsigned int, _id)
	)
	static unsigned int _idCntr;
};

void swap(CgiHandler&, CgiHandler&) /* noexcept */;
ostream& operator<<(ostream&, const CgiHandler&);
