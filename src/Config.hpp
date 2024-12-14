// <GENERATED>
#pragma once /* Config.hpp */

#include <string> /* std::string */
#include <iostream> /* std::ostream */
#include <map> /* std::map */

#include "repr.hpp" /* repr<T> */
#include "helper.hpp"
#include "BaseConfig.hpp"
#include "ServerConfig.hpp"
#include "CgiHandler.hpp"

using std::map;
using std::string;
using std::ostream;

class Config : public BaseConfig {
public:
	// <generated>
		~Config(); // destructor; consider virtual if it's a base class
		Config(); // default constructor
		Config(const string&);
		Config(	const string&,
				const string&,
				unsigned int,
				const string&,
				const string&,
				bool,
				const vector<string>&,
				const map<string, CgiHandler>&,
				const string&,
				const vector<ServerConfig>&,
				unsigned int); // serializing constructor
		Config(const Config&); // copy constructor
		Config& operator=(Config); // copy-assignment operator
		void swap(Config&); // copy-swap idiom
		string repr() const; // return string-serialized version of the object
		operator string() const; // convert object to string

		const string& getConfigPath() const;
		const vector<ServerConfig>& getServerConfigs() const;
		unsigned int getMaxWorkers() const;

		void setConfigPath(const string&);
		void setServerConfigs(const vector<ServerConfig>&);
		void setMaxWorkers(unsigned int);

		template <typename T>
		Config(const T& type, DeleteOverload = 0); // disallow accidental casting/conversion
	// </generated>
protected:
	string _configPath;
	vector<ServerConfig> _workerConfigs;
	unsigned int _maxWorkers;
private:
	unsigned int _id;
	static unsigned int _idCntr;

	void parse();
};

template <> inline string repr(const Config& value) { return value.repr(); }
void swap(Config&, Config&) /* noexcept */;
ostream& operator<<(ostream&, const Config&);
// </GENERATED>
