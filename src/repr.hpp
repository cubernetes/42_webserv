#pragma once /* repr.hpp */

#include <sstream> /* std::ostringstream */
#include <string> /* std::string */
#include <vector> /* std::vector */
#include <map> /* std::map */
#include <utility> /* std::pair */
#include "Logger.hpp" /* Logger::debug() */
#include "Constants.hpp" /* Constants::{no_color,repr_json} */

# define ANSI_CSI "\x1b\x5b"
# define ANSI_FG ANSI_CSI "48;2;41;41;41" "m"
# define ANSI_RST_FR ANSI_CSI "0" "m"
# define ANSI_RST    ANSI_CSI "0" "m" ANSI_FG
# define ANSI_STR    ANSI_FG ANSI_CSI "38;2;184;187;38"  "m"
# define ANSI_CHR    ANSI_FG ANSI_CSI "38;2;211;134;155" "m"
# define ANSI_KWRD   ANSI_FG ANSI_CSI "38;2;250;189;47"  "m"
# define ANSI_PUNCT  ANSI_FG ANSI_CSI "38;2;254;128;25"  "m"
# define ANSI_FUNC   ANSI_FG ANSI_CSI "38;2;184;187;38"  "m"
# define ANSI_NUM    ANSI_FG ANSI_CSI "38;2;211;134;155" "m"
# define ANSI_VAR    ANSI_FG ANSI_CSI "38;2;235;219;178" "m"
# define ANSI_CMT    ANSI_FG ANSI_CSI "38;2;146;131;116" "m"

using std::string;
using std::vector;
using std::map;
using std::pair;

void repr_init();
void repr_done();

// generic template, works for ints, floats, and some other fundamental types
// repr should be specialized or overridden for custom classes/non-fundamental types
// using C++ concepts, we should make this function only take integer template argument types
// Also, it needs to be wrapped in a struct/class, otherwise partial specializations don't work
template <typename T>
struct repr_wrapper {
	static inline string
	str(const T& value) {
		std::ostringstream oss;
		oss << ANSI_NUM << value << ANSI_RST;
		return oss.str();
	}
};

///// repr partial template specializations

// not escaping str value atm
template <>
struct repr_wrapper<string> {
	static inline string
	str(const string& value) {
		if (REPR_JSON)
			return string("\"") + value + "\"";
		else
			return ANSI_STR "\"" + value + "\"" + (Logger::debug() ? ANSI_PUNCT "s": "") + ANSI_RST;
	}
};

// not escaping str value atm
template <>
struct repr_wrapper<char*> {
	static inline string
	str(const char* const& value) {
		if (REPR_JSON)
			return string("\"") + value + "\"";
		else
			return string(ANSI_STR "\"") + value + "\"" ANSI_STR;
	}
};

// not escaping char value atm
template <>
struct repr_wrapper<char> {
	static inline string
	str(const char& value) {
		if (REPR_JSON)
			return string("\"") + value + "\"";
		else
			return string(ANSI_CHR "'") + value + "'" ANSI_RST;
	}
};

// for vector
template <typename T>
struct repr_wrapper<vector<T> > {
	static inline string
	str(const vector<T>& value) {
		std::ostringstream oss;
		if (REPR_JSON)
			oss << "[";
		else if (Logger::debug())
			oss << ANSI_KWRD "std" ANSI_PUNCT "::" ANSI_KWRD "vector" ANSI_PUNCT "({" ANSI_RST;
		else
			oss << ANSI_PUNCT "[" ANSI_RST;
		for (unsigned int i = 0; i < value.size(); ++i) {
			if (i != 0) {
				if (REPR_JSON)
					oss << ", ";
				else
					oss << ANSI_PUNCT ", " ANSI_RST;
			}
			oss << repr_wrapper<T>::str(value[i]);
		}
		if (REPR_JSON)
			oss << "]";
		else if (Logger::debug())
			oss << ANSI_PUNCT "})" ANSI_RST;
		else
			oss << ANSI_PUNCT "]" ANSI_RST;
		return oss.str();
	}
};

// for map
template <typename K, typename V>
struct repr_wrapper<map<K, V> > {
	static inline string
	str(const map<K, V>& m) {
		std::ostringstream oss;
		if (REPR_JSON)
			oss << "{";
		else if (Logger::debug())
			oss << ANSI_KWRD "std" ANSI_PUNCT "::" ANSI_KWRD "map" ANSI_PUNCT "({" ANSI_RST;
		else
			oss << ANSI_PUNCT "{" ANSI_RST;
		int i = 0;
		for (typename map<K, V>::const_iterator it = m.begin(); it != m.end(); ++it) {
			if (i != 0) {
				if (REPR_JSON)
					oss << ", ";
				else
					oss << ANSI_PUNCT ", " ANSI_RST;
			}
			oss << repr_wrapper<K>::str(it->first);
			if (REPR_JSON)
				oss << ": ";
			else
				oss << ANSI_PUNCT ": ";
			oss << repr_wrapper<V>::str(it->second);
			++i;
		}
		if (REPR_JSON)
			oss << "}";
		else if (Logger::debug())
			oss << ANSI_PUNCT "})" ANSI_RST;
		else
			oss << ANSI_PUNCT "}" ANSI_RST;
		return oss.str();
	}
};

// for pair
template <typename F, typename S>
struct repr_wrapper<pair<F, S> > {
	static inline string
	str(const pair<F, S>& p) {
		std::ostringstream oss;
		if (REPR_JSON)
			oss << "[";
		else if (Logger::debug())
			oss << ANSI_KWRD "std" ANSI_PUNCT "::" ANSI_KWRD "pair" ANSI_PUNCT "(" ANSI_RST;
		else
			oss << ANSI_PUNCT "(" ANSI_RST;
		oss << repr_wrapper<F>::str(p.first) << (REPR_JSON ? ", " : ANSI_PUNCT ", ") << repr_wrapper<S>::str(p.second);
		if (REPR_JSON)
			oss << "]";
		else
		oss << ANSI_PUNCT ")" ANSI_RST;
		return oss.str();
	}
};

// convenience wrapper
template <typename T>
static inline string
repr(const T& value) {
	return repr_wrapper<T>::str(value);
}

// convenience wrapper for arrays
template <typename T>
static inline string
repr(const T* value, unsigned int size) {
	std::ostringstream oss;
	if (REPR_JSON)
		oss << "[";
	else if (Logger::debug())
		oss << ANSI_PUNCT "{" ANSI_RST;
	else
		oss << ANSI_PUNCT "[" ANSI_RST;
	for (unsigned int i = 0; i < size; ++i) {
		if (i != 0) {
			if (REPR_JSON)
				oss << ", ";
			else
				oss << ANSI_PUNCT ", " ANSI_RST;
		}
		oss << repr_wrapper<T>::str(value[i]);
	}
	if (REPR_JSON)
		oss << "]";
	else if (Logger::debug())
		oss << ANSI_PUNCT "}" ANSI_RST;
	else
		oss << ANSI_PUNCT "]" ANSI_RST;
	return oss.str();
}
