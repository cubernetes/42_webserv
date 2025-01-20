#pragma once /* Repr.hpp */

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <sys/poll.h>

#include "Constants.hpp"
#include "Ansi.hpp"
#include "Reflection.hpp"

# define ANSI_FG     "41;41;41"
# define ANSI_STR    "184;187;38"
# define ANSI_CHR    "211;134;155"
# define ANSI_KWRD   "250;189;47"
# define ANSI_PUNCT  "254;128;25"
# define ANSI_FUNC   "184;187;38"
# define ANSI_NUM    "211;134;155"
# define ANSI_VAR    "235;219;178"
# define ANSI_CMT    "146;131;116"

using std::string;
using std::vector;
using std::map;
using std::pair;
using ansi::rgb;
using ansi::rgbBg;

namespace ReprClr {
	string str(string s);
	string chr(string s);
	string kwrd(string s);
	string punct(string s);
	string func(string s);
	string num(string s);
	string var(string s);
	string cmt(string s);
}

using ReprClr::str;
using ReprClr::chr;
using ReprClr::kwrd;
using ReprClr::punct;
using ReprClr::func;
using ReprClr::num;
using ReprClr::var;
using ReprClr::cmt;

void reprInit();
void reprDone();

template <typename T>
struct ReprWrapper {
	static inline string
	repr(const T& value, bool json = false) {
		return value.repr(json);
	}
};

template <class T>
static inline string getClass(const T& v) { (void)v; return "(unknown)"; }

// convenience wrapper
template <typename T>
static inline string
repr(const T& value, bool json = false) {
	return ReprWrapper<T>::repr(value, json);
}

// convenience wrapper for arrays with size
template <typename T>
static inline string
reprArr(const T* value, unsigned int size, bool json = false) {
	std::ostringstream oss;
	if (json)
		oss << "[";
	else if (Constants::verboseLogs)
		oss << punct("{");
	else
		oss << punct("[");
	for (unsigned int i = 0; i < size; ++i) {
		if (i != 0) {
			if (json)
				oss << ", ";
			else
				oss << punct(", ");
		}
		oss << ReprWrapper<T>::repr(value[i], json);
	}
	if (json)
		oss << "]";
	else if (Constants::verboseLogs)
		oss << punct("}");
	else
		oss << punct("]");
	return oss.str();
}

///// repr partial template specializations

#define INT_REPR(T) template <> struct ReprWrapper<T> { \
		static inline string \
		repr(const T& value, bool json = false) { \
			std::ostringstream oss; \
			oss << value; \
			if (json) \
				return oss.str(); \
			else \
				return num(oss.str()); \
		} \
	}

INT_REPR(int);
INT_REPR(short);
INT_REPR(long);
INT_REPR(unsigned int);
INT_REPR(unsigned short);
INT_REPR(unsigned long);
INT_REPR(float);
INT_REPR(double);
INT_REPR(long double);

template <> struct ReprWrapper<bool> {
	static inline string
	repr(const bool& value, bool json = false) {
		if (json) return value ? "true" : "false";
		else return num(value ? "true" : "false");
	}
};

// TODO: @timo: escape for string literal
template <>
struct ReprWrapper<string> {
	static inline string
	repr(const string& value, bool json = false) {
		if (json)
			return "\"" + value + "\"";
		else
			return str("\"" + value + "\"") + (Constants::verboseLogs ? punct("s") : "");
	}
};

// TODO: @timo: escape for string literal
template <> 
struct ReprWrapper<char*> {
	static inline string
	repr(const char* const& value, bool json = false) {
		if (json)
			return string("\"") + value + "\"";
		else
			return str(string("\"") + value + "\"");
	}
};

// TODO: @timo: escape for char literal
#define CHAR_REPR(T) template <> \
	struct ReprWrapper<T> { \
		static inline string \
		repr(const T& value, bool json = false) { \
			if (json) \
				return string("\"") + static_cast<char>(value) + "\""; \
			else \
				return chr(string("'") + static_cast<char>(value) + "'"); \
		} \
	}

CHAR_REPR(char);
CHAR_REPR(unsigned char);
CHAR_REPR(signed char);

#define MAKE_MEMBER_INIT_LIST(_, name) , name()
#define MAKE_DECL(type, name) type name;
#define MAKE_REPR_FN(_, name) string CAT(repr_, name)(bool json) const { return ::repr(name, json); }
#define MAKE_ASSIGN_GETTER(_, name) singleton.name = value.CAT(get, name)();
#define MAKE_ASSIGN_MEMBER(_, name) singleton.name = value.name;
#define MAKE_REFLECT(_, name) members[#name] = std::make_pair((ReprClosure)&ReprWrapper::CAT(repr_, name), &singleton.name);
#define POST_REFLECT_GETTER(clsId, ...) \
	static inline string getClass(const clsId& v) { (void)v; return #clsId; } \
	template <> \
	struct ReprWrapper<clsId> : public Reflection { \
		void reflect() {} \
		ReprWrapper() : uniqueNameMustComeFirst() FOR_EACH_PAIR(MAKE_MEMBER_INIT_LIST, __VA_ARGS__) {} \
		int uniqueNameMustComeFirst; \
		FOR_EACH_PAIR(MAKE_DECL, __VA_ARGS__) \
		FOR_EACH_PAIR(MAKE_REPR_FN, __VA_ARGS__) \
		static inline string \
		repr(const clsId& value, bool json = false) { \
			static ReprWrapper<clsId> singleton; \
			FOR_EACH_PAIR(MAKE_ASSIGN_GETTER, __VA_ARGS__) \
			Members members; \
			FOR_EACH_PAIR(MAKE_REFLECT, __VA_ARGS__) \
			return singleton.reprStruct(#clsId, members, json); \
		} \
	}
#define POST_REFLECT_MEMBER(clsId, ...) \
	static inline string getClass(const clsId& v) { (void)v; return #clsId; } \
	template <> \
	struct ReprWrapper<clsId> : public Reflection { \
		void reflect() {} \
		ReprWrapper() : uniqueNameMustComeFirst() FOR_EACH_PAIR(MAKE_MEMBER_INIT_LIST, __VA_ARGS__) {} \
		int uniqueNameMustComeFirst; \
		FOR_EACH_PAIR(MAKE_DECL, __VA_ARGS__) \
		FOR_EACH_PAIR(MAKE_REPR_FN, __VA_ARGS__) \
		static inline string \
		repr(const clsId& value, bool json = false) { \
			static ReprWrapper<clsId> singleton; \
			FOR_EACH_PAIR(MAKE_ASSIGN_MEMBER, __VA_ARGS__) \
			Members members; \
			FOR_EACH_PAIR(MAKE_REFLECT, __VA_ARGS__) \
			return singleton.reprStruct(#clsId, members, json); \
		} \
	}

POST_REFLECT_MEMBER(struct pollfd, int, fd, short, events, short, revents);

#include "HttpServer.hpp"
POST_REFLECT_GETTER(HttpServer, int, _serverFd, vector<struct pollfd>, _pollFds, bool, _running, Config, _config, unsigned int, _id);

#include "Server.hpp"
POST_REFLECT_GETTER(Server, unsigned int, _exitStatus, string, _rawConfig, Config, _config, HttpServer, _http, unsigned int, _id);

#include "CgiHandler.hpp"
POST_REFLECT_GETTER(CgiHandler, string, _extension, string, _program);

// for vector
template <typename T>
struct ReprWrapper<vector<T> > {
	static inline string
	repr(const vector<T>& value, bool json = false) {
		std::ostringstream oss;
		if (json)
			oss << "[";
		else if (Constants::verboseLogs)
			oss << kwrd("std") + punct("::") + kwrd("vector") + punct("({");
		else
			oss << punct("[");
		for (unsigned int i = 0; i < value.size(); ++i) {
			if (i != 0) {
				if (json)
					oss << ", ";
				else
					oss << punct(", ");
			}
			oss << ReprWrapper<T>::repr(value[i], json);
		}
		if (json)
			oss << "]";
		else if (Constants::verboseLogs)
			oss << punct("})");
		else
			oss << punct("]");
		return oss.str();
	}
};

// for map
template <typename K, typename V>
struct ReprWrapper<map<K, V> > {
	static inline string
	repr(const map<K, V>& m, bool json = false) {
		std::ostringstream oss;
		if (json)
			oss << "{";
		else if (Constants::verboseLogs)
			oss << kwrd("std") + punct("::") + kwrd("map") + punct("({");
		else
			oss << punct("{");
		int i = 0;
		for (typename map<K, V>::const_iterator it = m.begin(); it != m.end(); ++it) {
			if (i != 0) {
				if (json)
					oss << ", ";
				else
					oss << punct(", ");
			}
			oss << ReprWrapper<K>::repr(it->first, json);
			if (json)
				oss << ": ";
			else
				oss << punct(": ");
			oss << ReprWrapper<V>::repr(it->second, json);
			++i;
		}
		if (json)
			oss << "}";
		else if (Constants::verboseLogs)
			oss << punct("})");
		else
			oss << punct("}");
		return oss.str();
	}
};

// for pair
template <typename F, typename S>
struct ReprWrapper<pair<F, S> > {
	static inline string
	repr(const pair<F, S>& p, bool json = false) {
		std::ostringstream oss;
		if (json)
			oss << "[";
		else if (Constants::verboseLogs)
			oss << kwrd("std") + punct("::") + kwrd("pair") + punct("(");
		else
			oss << punct("(");
		oss << ReprWrapper<F>::repr(p.first, json) << (json ? ", " : punct(", ")) << ReprWrapper<S>::repr(p.second, json);
		if (json)
			oss << "]";
		else
		oss << punct(")");
		return oss.str();
	}
};

// to print using `std::cout << ...'
template<typename T>
static inline std::ostream& operator<<(std::ostream& os, const vector<T>& val) { return os << repr(val, Constants::jsonTrace); }

template<typename K, typename V>
static inline std::ostream& operator<<(std::ostream& os, const map<K, V>& val) { return os << repr(val, Constants::jsonTrace); }

template<typename F, typename S>
static inline std::ostream& operator<<(std::ostream& os, const pair<F, S>& val) { return os << repr(val, Constants::jsonTrace); }

static inline std::ostream& operator<<(std::ostream& os, const struct pollfd& val) { return os << repr(val, Constants::jsonTrace); }
