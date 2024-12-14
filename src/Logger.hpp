/* // <GENERATED> */
#pragma once /* Logger.hpp */

#include <exception>
#include <string> /* std::string */
#include <iostream> /* std::ostream */

#include "repr.hpp" /* repr<T> */
#include "helper.hpp"

using std::string;
using std::ostream;

class Logger {
public:
	// <generated>
		~Logger(); // destructor; consider virtual if it's a base class
		Logger(); // default constructor
		Logger(const Logger&); // copy constructor
		Logger& operator=(Logger); // copy-assignment operator
		void swap(Logger&); // copy-swap idiom
		string repr() const; // return string-serialized version of the object
		operator string() const; // convert object to string

		template <typename T>
		Logger(const T& type, DeleteOverload = 0); // disallow accidental casting/conversion
	// </generated>
	
	static void logexception(const std::exception& exception);
	static void logerror(const char* error);
	static const int logLevelTrace = 0;
	static const int logLevelDebug = 1;
	static const int logLevelInfo  = 2;
	static const int logLevelWarn  = 3;
	static const int logLevelError = 4;
	static const int logLevelFatal = 5;
	static bool trace();
	static bool debug();
	static bool info();
	static bool warn();
	static bool error();
	static bool fatal();
private:
	unsigned int _id;
	static unsigned int _idCntr;
};

template <> inline string repr(const Logger& value) { return value.repr(); }
void swap(Logger&, Logger&) /* noexcept */;
ostream& operator<<(ostream&, const Logger&);
// </GENERATED>
