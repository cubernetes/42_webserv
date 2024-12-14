// <GENERATED>
#pragma once /* CgiHandler.hpp */

#include <string> /* std::string */
#include <iostream> /* std::ostream */

#include "repr.hpp" /* repr<T> */
#include "helper.hpp"

using std::string;
using std::ostream;

class CgiHandler {
public:
	// <generated>
		~CgiHandler(); // destructor; consider virtual if it's a base class
		CgiHandler(); // default constructor
		explicit CgiHandler(const string&);
		explicit CgiHandler(const char*);
		CgiHandler(const string&, const string&); // serializing constructor
		CgiHandler(const CgiHandler&); // copy constructor
		CgiHandler& operator=(CgiHandler); // copy-assignment operator
		void swap(CgiHandler&); // copy-swap idiom
		string repr() const; // return string-serialized version of the object
		operator string() const; // convert object to string

		const string& get_extension() const;
		const string& get_program() const;

		void set_extension(const string&);
		void set_program(const string&);

		template <typename T>
		CgiHandler(const T& type, DeleteOverload = 0); // disallow accidental casting/conversion
	// </generated>
private:
	string _extension;
	string _program;
	unsigned int _id;
	static unsigned int _idCntr;
};

template <> inline string repr(const CgiHandler& value) { return value.repr(); }
void swap(CgiHandler&, CgiHandler&) /* noexcept */;
ostream& operator<<(ostream&, const CgiHandler&);
// </GENERATED>
