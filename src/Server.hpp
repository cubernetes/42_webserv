// <GENERATED>
#pragma once /* Server.hpp */

#include <string> /* std::string */
#include <iostream> /* std::ostream */

#include "repr.hpp" /* repr<T> */
#include "helper.hpp"
#include "Config.hpp"

using std::string;
using std::ostream;

class Server {
public:
	// <generated>
		~Server(); // destructor; consider virtual if it's a base class
		Server(); // default constructor
		Server(unsigned int, const Config&); // serializing constructor
		Server(const Server&); // copy constructor
		Server& operator=(Server); // copy-assignment operator
		void swap(Server&); // copy-swap idiom
		string repr() const; // return string-serialized version of the object
		operator string() const; // convert object to string

		int getExitStatus() const;
		const Config& get_config() const;

		void setExitStatus(int);
		void set_config(const Config&);

		template <typename T>
		Server(const T& type, DeleteOverload = 0); // disallow accidental casting/conversion
	// </generated>

	void serve();

	unsigned int exitStatus;
private:
	Config _config;
	unsigned int _id;
	static unsigned int _idCntr;
};

template <> inline string repr(const Server& value) { return value.repr(); }
void swap(Server&, Server&) /* noexcept */;
ostream& operator<<(ostream&, const Server&);
// </GENERATED>
