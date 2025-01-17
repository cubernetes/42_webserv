#pragma once /* CgiHandler.hpp */

#include <string> /* std::string */
#include <iostream> /* std::ostream */

using std::string;
using std::ostream;

class CgiHandler {
public:
		~CgiHandler();
		CgiHandler();
		CgiHandler(const CgiHandler& other);
		CgiHandler& operator=(CgiHandler);
		void swap(CgiHandler&); // copy swap idiom

		// string conversion
		operator string() const;

		const string& get_extension() const;
		const string& get_program() const;
private:
	string _extension;
	string _program;
};

void swap(CgiHandler&, CgiHandler&) /* noexcept */;
ostream& operator<<(ostream&, const CgiHandler&);
