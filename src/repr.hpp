#pragma once /* repr.hpp */

#include <sstream> /* std::ostringstream */
#include <string> /* std::string */
#include <vector> /* std::vector */
#include <map> /* std::map */
#include <utility> /* std::pair */
#include <sys/poll.h> /* struct pollfd */
#include <utility> /* std::make_pair */

#include "Logger.hpp" /* Logger::debug() */
#define LOGGER_DEBUG Logger::debug()

#include "Constants.hpp" /* Constants::{no_color,repr_json} */
#include "ansi.hpp"
#include "Reflection.hpp"

# define ANSI_CSI "\x1b\x5b"
# define ANSI_FG     41,41,41
# define ANSI_RST_FR ANSI_CSI "0" "m"
// # define ANSI_RST    ANSI_CSI "0" "m" ANSI_FG
# define ANSI_STR    184,187,38
# define ANSI_CHR    211,134,155
# define ANSI_KWRD   250,189,47
# define ANSI_PUNCT  254,128,25
# define ANSI_FUNC   184,187,38
# define ANSI_NUM    211,134,155
# define ANSI_VAR    235,219,178
# define ANSI_CMT    146,131,116

using std::string;
using std::vector;
using std::map;
using std::pair;
using ansi::rgb;
using ansi::rgb_bg;
using std::ostream;

void repr_init();
void repr_done();

namespace repr_clr {
	string str(string s);
	string chr(string s);
	string kwrd(string s);
	string punct(string s);
	string func(string s);
	string num(string s);
	string var(string s);
	string cmt(string s);
}

using repr_clr::str;
using repr_clr::chr;
using repr_clr::kwrd;
using repr_clr::punct;
using repr_clr::func;
using repr_clr::num;
using repr_clr::var;
using repr_clr::cmt;

template <typename T>
struct repr_wrapper {
	static inline string
	repr(const T& value, bool json = false) {
		return value.repr(json);
	}
};

// convenience wrapper
template <typename T>
static inline string
repr(const T& value, bool json = false) {
	return repr_wrapper<T>::repr(value, json);
}

// convenience wrapper for arrays with size
template <typename T>
static inline string
repr(const T* value, unsigned int size, bool json = false) {
	std::ostringstream oss;
	if (json)
		oss << "[";
	else if (LOGGER_DEBUG)
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
		oss << repr_wrapper<T>::repr(value[i], json);
	}
	if (json)
		oss << "]";
	else if (LOGGER_DEBUG)
		oss << punct("}");
	else
		oss << punct("]");
	return oss.str();
}

///// repr partial template specializations

#define INT_REPR(T) template <> struct repr_wrapper<T> { \
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
INT_REPR(long long);
INT_REPR(unsigned int);
INT_REPR(unsigned short);
INT_REPR(unsigned long);
INT_REPR(unsigned long long);
INT_REPR(float);
INT_REPR(double);
INT_REPR(long double);

template <> struct repr_wrapper<bool> {
	static inline string
	repr(const bool& value, bool json = false) {
		if (json) return value ? "true" : "false";
		else return num(value ? "true" : "false");
	}
};

// not escaping string value atm
template <>
struct repr_wrapper<string> {
	static inline string
	repr(const string& value, bool json = false) {
		if (json)
			return "\"" + value + "\"";
		else
			return str("\"" + value + "\"") + (LOGGER_DEBUG ? punct("s") : "");
	}
};

// not escaping string value atm
template <> 
struct repr_wrapper<char*> {
	static inline string
	repr(const char* const& value, bool json = false) {
		if (json)
			return string("\"") + value + "\"";
		else
			return str(string("\"") + value + "\"");
	}
};

// not escaping char value atm
#define CHAR_REPR(T) template <> \
	struct repr_wrapper<T> { \
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
#define MAKE_REPR_FN(_, name) string CAT(repr_, name)(bool json) { return ::repr(name, json); }
#define MAKE_ASSIGN(_, name) singleton.name = value.name;
#define MAKE_REFLECT(_, name) members[#name] = std::make_pair((t_repr_closure)&repr_wrapper::CAT(repr_, name), &singleton.name);
#define POST_REFLECT(cls_id, ...) \
	template <> \
	struct repr_wrapper<cls_id> : public Reflection { \
		void reflect() {} \
		repr_wrapper() : __nop___unique() FOR_EACH_PAIR(MAKE_MEMBER_INIT_LIST, __VA_ARGS__) {} \
		int __nop___unique; \
		FOR_EACH_PAIR(MAKE_DECL, __VA_ARGS__) \
		FOR_EACH_PAIR(MAKE_REPR_FN, __VA_ARGS__) \
		static inline string \
		repr(const cls_id& value, bool json = false) { \
			static repr_wrapper<cls_id> singleton; \
			FOR_EACH_PAIR(MAKE_ASSIGN, __VA_ARGS__) \
			t_members members; \
			FOR_EACH_PAIR(MAKE_REFLECT, __VA_ARGS__) \
			return singleton.repr_struct(#cls_id, members, json); \
		} \
	}

POST_REFLECT(struct pollfd, int, fd, short, events, short, revents);

// for vector
template <typename T>
struct repr_wrapper<vector<T> > {
	static inline string
	repr(const vector<T>& value, bool json = false) {
		std::ostringstream oss;
		if (json)
			oss << "[";
		else if (LOGGER_DEBUG)
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
			oss << repr_wrapper<T>::repr(value[i], json);
		}
		if (json)
			oss << "]";
		else if (LOGGER_DEBUG)
			oss << punct("})");
		else
			oss << punct("]");
		return oss.str();
	}
};

// for map
template <typename K, typename V>
struct repr_wrapper<map<K, V> > {
	static inline string
	repr(const map<K, V>& m, bool json = false) {
		std::ostringstream oss;
		if (json)
			oss << "{";
		else if (LOGGER_DEBUG)
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
			oss << repr_wrapper<K>::repr(it->first, json);
			if (json)
				oss << ": ";
			else
				oss << punct(": ");
			oss << repr_wrapper<V>::repr(it->second, json);
			++i;
		}
		if (json)
			oss << "}";
		else if (LOGGER_DEBUG)
			oss << punct("})");
		else
			oss << punct("}");
		return oss.str();
	}
};

// for pair
template <typename F, typename S>
struct repr_wrapper<pair<F, S> > {
	static inline string
	repr(const pair<F, S>& p, bool json = false) {
		std::ostringstream oss;
		if (json)
			oss << "[";
		else if (LOGGER_DEBUG)
			oss << kwrd("std") + punct("::") + kwrd("pair") + punct("(");
		else
			oss << punct("(");
		oss << repr_wrapper<F>::repr(p.first, json) << (json ? ", " : punct(", ")) << repr_wrapper<S>::repr(p.second, json);
		if (json)
			oss << "]";
		else
		oss << punct(")");
		return oss.str();
	}
};

// to print using `std::cout << ...'
template<typename T>
static inline ostream& operator<<(ostream& os, const vector<T>& val) { return os << repr(val, Constants::jsonTrace); }

template<typename K, typename V>
static inline ostream& operator<<(ostream& os, const map<K, V>& val) { return os << repr(val, Constants::jsonTrace); }

template<typename F, typename S>
static inline ostream& operator<<(ostream& os, const pair<F, S>& val) { return os << repr(val, Constants::jsonTrace); }
