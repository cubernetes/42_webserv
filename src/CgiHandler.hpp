#pragma once /* CgiHandler.hpp */

#include <string> /* std::string */
#include <iostream> /* std::ostream */

// #include "Reflection.hpp"
// #include "repr.hpp"

using std::string;
using std::ostream;

class CgiHandler /* : public Reflection */ {
public:
		~CgiHandler(); // destructor; consider virtual if it's a base class
		CgiHandler(); // default constructor
		explicit CgiHandler(const string&); // TODO: do we need?
		explicit CgiHandler(const char*); // TODO: do we need?
		CgiHandler(const string& extension, const string& program); // serializing constructor
		CgiHandler(const CgiHandler& other); // copy constructor
		CgiHandler& operator=(CgiHandler); // copy-assignment operator
		void swap(CgiHandler&); // copy-swap idiom
		operator string() const; // convert object to string

		const string& get_extension() const; // TODO: can't we just make them public?
		const string& get_program() const; // TODO: can't we just make them public?

		void set_extension(const string&); // TODO: can't we just make them public?
		void set_program(const string&); // TODO: can't we just make them public?
private:
	// REFLECT(
	// 	"CgiHandler",
	// 	DECL(string, _extension),
	// 	DECL(string, _program),
	// 	DECL(unsigned int, _id)
	// )
	string _extension;
	string _program;
	unsgined int _id;

	static unsigned int _idCntr;
};

void swap(CgiHandler&, CgiHandler&) /* noexcept */;
ostream& operator<<(ostream&, const CgiHandler&);
