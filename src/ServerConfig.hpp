// <GENERATED>
#pragma once /* ServerConfig.hpp */

#include <string> /* std::string */
#include <iostream> /* std::ostream */
#include <map> /* std::map */

#include "repr.hpp" /* repr<T> */
#include "helper.hpp"
#include "BaseConfig.hpp"
#include "CgiHandler.hpp"

using std::map;
using std::string;
using std::ostream;

class ServerConfig : public BaseConfig {
public:
	// <generated>
		~ServerConfig(); // destructor; consider virtual if it's a base class
		ServerConfig(); // default constructor
		ServerConfig(	const string&,
						const string&,
						unsigned int,
						const string&,
						const string&,
						bool,
						const vector<string>&,
						const map<string, CgiHandler>&,
						const string&,
						const string&,
						unsigned int); // serializing constructor
		ServerConfig(const ServerConfig&); // copy constructor
		ServerConfig& operator=(ServerConfig); // copy-assignment operator
		void swap(ServerConfig&); // copy-swap idiom
		string repr() const; // return string-serialized version of the object
		operator string() const; // convert object to string

		const string& getServerName() const;
		const string& getListenAddress() const;
		unsigned int getListenPort() const;

		void setServerName(const string&);
		void setListenAddress(const string&);
		void setListenPort(unsigned int);

		template <typename T>
		ServerConfig(const T& type, DeleteOverload = 0); // disallow accidental casting/conversion
	// </generated>
protected:
	string _serverName;
	string _listenAddress;
	unsigned int _listenPort;
private:
	unsigned int _id;
	static unsigned int _idCntr;
};

template <> inline string repr(const ServerConfig& value) { return value.repr(); }
void swap(ServerConfig&, ServerConfig&) /* noexcept */;
ostream& operator<<(ostream&, const ServerConfig&);
// </GENERATED>
