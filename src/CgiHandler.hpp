#pragma once /* CgiHandler.hpp */

#include <ostream>
#include <string>

using std::string;

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
std::ostream& operator<<(std::ostream&, const CgiHandler&);
