// <GENERATED>
#include <iostream> /* std::cout, std::swap, std::ostream */
#include <string> /* std::string */
#include <sstream> /* std::stringstream */
#include <vector> /* std::vector */
#include <map> /* std::map */

#include "repr.hpp"
#include "BaseConfig.hpp"
#include "CgiHandler.hpp"
#include "Logger.hpp"

using std::cout;
using std::vector;
using std::map;
using std::swap;
using std::string;
using std::ostream;
using std::stringstream;

// De- & Constructors
BaseConfig::~BaseConfig() {
	if (Logger::trace())
		cout << ANSI_PUNCT "~" << *this << '\n';
}

BaseConfig::BaseConfig() :
		_accessLog(),
		_errorLog(),
		_maxBodySize(),
		_root(),
		_errorPage(),
		_enableDirListing(),
		_indexFiles(),
		_cgiHandlers(),
		_id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "BaseConfig" ANSI_PUNCT "() -> " << *this << '\n';
}

BaseConfig::BaseConfig(
	const string& accessLog,
	const string& errorLog,
	unsigned int maxBodySize,
	const string& root,
	const string& errorPage,
	bool enableDirListing,
	const vector<string>& indexFiles,
	const map<string, CgiHandler>& cgiHandlers) :
		_accessLog(accessLog),
		_errorLog(errorLog),
		_maxBodySize(maxBodySize),
		_root(root),
		_errorPage(errorPage),
		_enableDirListing(enableDirListing),
		_indexFiles(indexFiles),
		_cgiHandlers(cgiHandlers),
		_id(_idCntr++) {
	if (Logger::trace())
		cout << *this << ANSI_PUNCT " -> " << *this << '\n';
}

BaseConfig::BaseConfig(const BaseConfig& other) :
		_accessLog(other._accessLog),
		_errorLog(other._errorLog),
		_maxBodySize(other._maxBodySize),
		_root(other._root),
		_errorPage(other._errorPage),
		_enableDirListing(other._enableDirListing),
		_indexFiles(other._indexFiles),
		_cgiHandlers(other._cgiHandlers),
		_id(_idCntr++) {
	if (Logger::trace())
		cout << ANSI_KWRD "BaseConfig" ANSI_PUNCT "(" << ::repr(other) << ANSI_PUNCT ") -> " << *this << '\n';
}

// Copy-assignment operator (using copy-swap idiom)
BaseConfig& BaseConfig::operator=(BaseConfig other) /* noexcept */ {
	if (Logger::trace())
		cout << ANSI_KWRD "BaseConfig" ANSI_PUNCT "& " ANSI_KWRD "BaseConfig" ANSI_PUNCT "::" ANSI_FUNC "operator" ANSI_PUNCT "=(" << ::repr(other) << ANSI_PUNCT ")" ANSI_RST "\n";
	::swap(*this, other);
	return *this;
}

// Generated getters
const string& BaseConfig::getAccessLog() const { return _accessLog; }
const string& BaseConfig::getErrorLog() const { return _errorLog; }
unsigned int BaseConfig::getMaxBodySize() const { return _maxBodySize; }
const string& BaseConfig::getRoot() const { return _root; }
const string& BaseConfig::getErrorPage() const { return _errorPage; }
bool BaseConfig::getEnableDirListing() const { return _enableDirListing; }
const vector<string>& BaseConfig::getIndexFiles() const { return _indexFiles; }
const map<string, CgiHandler>& BaseConfig::getCgiHandlers() const { return _cgiHandlers; }

// Generated setters
void BaseConfig::setAccessLog(const string& value) { _accessLog = value; }
void BaseConfig::setErrorLog(const string& value) { _errorLog = value; }
void BaseConfig::setMaxBodySize(unsigned int value) { _maxBodySize = value; }
void BaseConfig::setRoot(const string& value) { _root = value; }
void BaseConfig::setErrorPage(const string& value) { _errorPage = value; }
void BaseConfig::setEnableDirListing(bool value) { _enableDirListing = value; }
void BaseConfig::setIndexFiles(const vector<string>& value) { _indexFiles = value; }
void BaseConfig::setCgiHandlers(const map<string, CgiHandler>& value) { _cgiHandlers = value; }

// Generated member functions
string BaseConfig::repr() const {
	stringstream out;
	out << ANSI_KWRD "BaseConfig" ANSI_PUNCT "("
		<< ::repr(_accessLog) << ANSI_PUNCT ", "
		<< ::repr(_errorLog) << ANSI_PUNCT ", "
		<< ::repr(_maxBodySize) << ANSI_PUNCT ", "
		<< ::repr(_root) << ANSI_PUNCT ", "
		<< ::repr(_errorPage) << ANSI_PUNCT ", "
		<< ::repr(_enableDirListing) << ANSI_PUNCT ", "
		<< ::repr(_indexFiles) << ANSI_PUNCT ", "
		<< ::repr(_cgiHandlers) << ANSI_PUNCT ", "
		<< ::repr(_id) << ANSI_PUNCT ")" ANSI_RST;
	return out.str();
}

void BaseConfig::swap(BaseConfig& other) /* noexcept */ {
	if (Logger::trace()) {
		cout << ANSI_CMT "<Swapping BaseConfig *this:" ANSI_RST "\n";
		cout << *this << '\n';
		cout << ANSI_CMT "with the following BaseConfig object:" ANSI_RST "\n";
		cout << other << '\n';
	}
	::swap(_accessLog, other._accessLog);
	::swap(_errorLog, other._errorLog);
	::swap(_maxBodySize, other._maxBodySize);
	::swap(_root, other._root);
	::swap(_errorPage, other._errorPage);
	::swap(_enableDirListing, other._enableDirListing);
	::swap(_indexFiles, other._indexFiles);
	::swap(_cgiHandlers, other._cgiHandlers);
	::swap(_id, other._id);
	if (Logger::trace())
		cout << ANSI_CMT "BaseConfig swap done>" ANSI_RST "\n";
}

BaseConfig::operator string() const { return ::repr(*this); }

// Generated free functions
void swap(BaseConfig& a, BaseConfig& b) /* noexcept */ { a.swap(b); }
ostream& operator<<(ostream& os, const BaseConfig& other) { return os << static_cast<string>(other); }

// Keeping track of the instances
unsigned int BaseConfig::_idCntr = 0;
// </GENERATED>
