#pragma once /* repr.hpp */

#include <sstream> /* std::ostringstream */
#include <string> /* std::string */
#include <vector> /* std::vector */
#include <map> /* std::map */
#include <utility> /* std::pair */

#include "Logger.hpp" /* Logger::debug() */
#include "Constants.hpp" /* Constants::{no_color,repr_json} */
#include "ansi.hpp"

#ifndef REPR_JSON
# define REPR_JSON false
#endif

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

// generic template, works for ints, floats, and some other fundamental types
// repr should be specialized or overridden for custom classes/non-fundamental types
// using C++ concepts, we should make this function only take integer template argument types
// Also, it needs to be wrapped in a struct/class, otherwise partial specializations don't work
template <typename T>
struct repr_wrapper {
	static inline string
	repr(const T& value) {
		std::ostringstream oss;
		oss << value;
		return num(oss.str());
	}
};

///// repr partial template specializations

// not escaping string value atm
template <>
struct repr_wrapper<string> {
	static inline string
	repr(const string& value) {
		if (REPR_JSON)
			return "\"" + value + "\"";
		else
			return str("\"" + value + "\"") + (Logger::debug() ? punct("s") : "");
	}
};

// not escaping string value atm
template <>
struct repr_wrapper<char*> {
	static inline string
	repr(const char* const& value) {
		if (REPR_JSON)
			return string("\"") + value + "\"";
		else
			return str(string("\"") + value + "\"");
	}
};

// not escaping char value atm
template <>
struct repr_wrapper<char> {
	static inline string
	repr(const char& value) {
		if (REPR_JSON)
			return string("\"") + value + "\"";
		else
			return chr(string("'") + value + "'");
	}
};

// for vector
template <typename T>
struct repr_wrapper<vector<T> > {
	static inline string
	repr(const vector<T>& value) {
		std::ostringstream oss;
		if (REPR_JSON)
			oss << "[";
		else if (Logger::debug())
			oss << kwrd("std") + punct("::") + kwrd("vector") + punct("({");
		else
			oss << punct("[");
		for (unsigned int i = 0; i < value.size(); ++i) {
			if (i != 0) {
				if (REPR_JSON)
					oss << ", ";
				else
					oss << punct(", ");
			}
			oss << repr_wrapper<T>::repr(value[i]);
		}
		if (REPR_JSON)
			oss << "]";
		else if (Logger::debug())
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
	repr(const map<K, V>& m) {
		std::ostringstream oss;
		if (REPR_JSON)
			oss << "{";
		else if (Logger::debug())
			oss << kwrd("std") + punct("::") + kwrd("map") + punct("({");
		else
			oss << punct("{");
		int i = 0;
		for (typename map<K, V>::const_iterator it = m.begin(); it != m.end(); ++it) {
			if (i != 0) {
				if (REPR_JSON)
					oss << ", ";
				else
					oss << punct(", ");
			}
			oss << repr_wrapper<K>::repr(it->first);
			if (REPR_JSON)
				oss << ": ";
			else
				oss << punct(": ");
			oss << repr_wrapper<V>::repr(it->second);
			++i;
		}
		if (REPR_JSON)
			oss << "}";
		else if (Logger::debug())
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
	repr(const pair<F, S>& p) {
		std::ostringstream oss;
		if (REPR_JSON)
			oss << "[";
		else if (Logger::debug())
			oss << kwrd("std") + punct("::") + kwrd("pair") + punct("(");
		else
			oss << punct("(");
		oss << repr_wrapper<F>::repr(p.first) << (REPR_JSON ? ", " : punct(", ")) << repr_wrapper<S>::repr(p.second);
		if (REPR_JSON)
			oss << "]";
		else
		oss << punct(")");
		return oss.str();
	}
};

// convenience wrapper
template <typename T>
static inline string
repr(const T& value) {
	return repr_wrapper<T>::repr(value);
}

// convenience wrapper for arrays
template <typename T>
static inline string
repr(const T* value, unsigned int size) {
	std::ostringstream oss;
	if (REPR_JSON)
		oss << "[";
	else if (Logger::debug())
		oss << punct("{");
	else
		oss << punct("[");
	for (unsigned int i = 0; i < size; ++i) {
		if (i != 0) {
			if (REPR_JSON)
				oss << ", ";
			else
				oss << punct(", ");
		}
		oss << repr_wrapper<T>::repr(value[i]);
	}
	if (REPR_JSON)
		oss << "]";
	else if (Logger::debug())
		oss << punct("}");
	else
		oss << punct("]");
	return oss.str();
}
