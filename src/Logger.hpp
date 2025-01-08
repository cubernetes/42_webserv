/* // <GENERATED> */
#pragma once /* Logger.hpp */

#include <exception>
#include <string> /* std::string */
#include <iostream> /* std::ostream */

#include "Reflection.hpp"

using std::string;
using std::ostream;

class Logger : public Reflection {
public:
	// <generated>
		~Logger(); // destructor; consider virtual if it's a base class
		Logger(); // default constructor
		Logger(const Logger&); // copy constructor
		Logger& operator=(Logger); // copy-assignment operator
		void swap(Logger&); // copy-swap idiom
		operator string() const; // convert object to string
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
	REFLECT(
		"Logger",
		DECL(unsigned int, _id)
	)
	static unsigned int _idCntr;
};

template <typename T> struct repr_wrapper;
template <> struct repr_wrapper<Logger> { static inline string repr(const Logger& value, bool json = false) { return value.repr(json); } };
void swap(Logger&, Logger&) /* noexcept */;
ostream& operator<<(ostream&, const Logger&);
// </GENERATED>
