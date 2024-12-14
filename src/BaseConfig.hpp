// <GENERATED>
#pragma once /* BaseConfig.hpp */

#include <string> /* std::string */
#include <iostream> /* std::ostream */
#include <vector> /* std::vector */
#include <map> /* std::map */

#include "repr.hpp" /* repr<T> */
#include "CgiHandler.hpp"

using std::string;
using std::ostream;
using std::vector;
using std::map;

class BaseConfig {
public:
	// <generated>
		virtual ~BaseConfig(); // destructor; consider virtual if it's a base class
		BaseConfig(); // default constructor
		BaseConfig(const string&, const string&, unsigned int, const string&, const string&, bool, const vector<string>&, const map<string, CgiHandler>&); // serializing constructor
		BaseConfig(const BaseConfig&); // copy constructor
		BaseConfig& operator=(BaseConfig); // copy-assignment operator
		void swap(BaseConfig&); // copy-swap idiom
		string repr() const; // return string-serialized version of the object
		operator string() const; // convert object to string

		const string& getAccessLog() const;
		const string& getErrorLog() const;
		unsigned int getMaxBodySize() const;
		const string& getRoot() const;
		const string& getErrorPage() const;
		bool getEnableDirListing() const;
		const vector<string>& getIndexFiles() const;
		const map<string, CgiHandler>& getCgiHandlers() const;

		void setAccessLog(const string&);
		void setErrorLog(const string&);
		void setMaxBodySize(unsigned int);
		void setRoot(const string&);
		void setErrorPage(const string&);
		void setEnableDirListing(bool);
		void setIndexFiles(const vector<string>&);
		void setCgiHandlers(const map<string, CgiHandler>&);
	// </generated>
protected:
	string _accessLog;
	string _errorLog;
	unsigned int _maxBodySize;
	string _root;
	string _errorPage;
	bool _enableDirListing;
	vector<string> _indexFiles;
	map<string, CgiHandler> _cgiHandlers;
private:
	unsigned int _id;
	static unsigned int _idCntr;
};

template <> inline string repr(const BaseConfig& value) { return value.repr(); }
void swap(BaseConfig&, BaseConfig&) /* noexcept */;
ostream& operator<<(ostream&, const BaseConfig&);
// </GENERATED>
