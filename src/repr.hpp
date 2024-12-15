// <GENERATED>
#pragma once /* repr.hpp */

#include <sstream> /* std::ostringstream */
#include <string> /* std::string */
#include <vector> /* std::vector */
#include <map> /* std::map */

#define ANSI_CSI "\x1b\x5b"
#define ANSI_FG ANSI_CSI "48;2;41;41;41" "m"
#define ANSI_RST_FR ANSI_CSI "0" "m"
#define ANSI_RST    ANSI_CSI "0" "m" ANSI_FG
#define ANSI_STR    ANSI_FG ANSI_CSI "38;2;184;187;38"  "m"
#define ANSI_CHR    ANSI_FG ANSI_CSI "38;2;211;134;155" "m"
#define ANSI_KWRD   ANSI_FG ANSI_CSI "38;2;250;189;47"  "m"
#define ANSI_PUNCT  ANSI_FG ANSI_CSI "38;2;254;128;25"  "m"
#define ANSI_FUNC   ANSI_FG ANSI_CSI "38;2;184;187;38"  "m"
#define ANSI_NUM    ANSI_FG ANSI_CSI "38;2;211;134;155" "m"
#define ANSI_VAR    ANSI_FG ANSI_CSI "38;2;235;219;178" "m"
#define ANSI_CMT    ANSI_FG ANSI_CSI "38;2;146;131;116" "m"

using std::string;
using std::vector;
using std::map;

// generic template, works for ints, floats, and some other fundamental types
// repr should be specialized or overridden for custom classes
template <typename T>
inline string repr(const T& value) {
	std::ostringstream oss;
	oss << ANSI_NUM << value << ANSI_RST;
	return oss.str();
}

// repr template specializations (not escaping value atm)
template <> inline string repr(const string& value) { return ANSI_STR "\"" + value + "\"" ANSI_PUNCT "s" ANSI_RST; }
template <> inline string repr(const char* const& value) { return string(ANSI_STR "\"") + value + "\"" ANSI_STR; }
template <> inline string repr(const char& value) { return string(ANSI_CHR "'") + value + "'" ANSI_RST; }
template <typename T> inline string repr(const T* value, unsigned int size) {
	std::ostringstream oss;
	oss << ANSI_PUNCT "{" ANSI_RST;
	for (unsigned int i = 0; i < size; ++i) {
		if (i != 0)
			oss << ANSI_PUNCT ", " ANSI_RST;
		oss << ::repr(value[i]);
	}
	oss << ANSI_PUNCT "}" ANSI_RST;
	return oss.str();
}

template <typename T> inline string repr(const vector<T>& value) {
	std::ostringstream oss;
	oss << ANSI_KWRD "std" ANSI_PUNCT "::" ANSI_KWRD "vector" ANSI_PUNCT "({" ANSI_RST;
	for (unsigned int i = 0; i < value.size(); ++i) {
		if (i != 0)
			oss << ANSI_PUNCT ", " ANSI_RST;
		oss << ::repr(value[i]);
	}
	oss << ANSI_PUNCT "})" ANSI_RST;
	return oss.str();
}

void repr_init();
void repr_done();
// </GENERATED>

template <typename K, typename V> inline string repr(const map<K, V>& m) {
	std::ostringstream oss;
	oss << ANSI_KWRD "std" ANSI_PUNCT "::" ANSI_KWRD "map" ANSI_PUNCT "({" ANSI_RST;
	int i = 0;
	for (typename map<K, V>::const_iterator it = m.begin(); it != m.end(); ++it) {
		if (i != 0)
			oss << ANSI_PUNCT ", " ANSI_RST;
		oss << ::repr(it->first);
		oss << ANSI_PUNCT ": ";
		oss << ::repr(it->second);
		++i;
	}
	oss << ANSI_PUNCT "})" ANSI_RST;
	return oss.str();
}
