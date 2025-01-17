#include <sstream>
#include <iostream>
#include <string>
#include <map>
#include <utility>
#include <cstddef>
#include <algorithm>

#include "Repr.hpp"
#include "Reflection.hpp"
#include "Logger.hpp"
#include "HttpServer.hpp"

using std::string;
using std::map;
using std::pair;
using std::cout;
using std::swap;

static string _replace(string s, const string& search,
                          const string& replace) {
	std::size_t pos = 0;
    while ((pos = s.find(search, pos)) != string::npos) {
         s.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return s;
}

static string jsonEscape(string s) {
	return _replace(_replace(s, "\\", "\\\\"), "\"", "\\\"");
}

Reflection::Reflection() : _class(), _members() {}
Reflection::Reflection(const Reflection& other) : _class(other._class), _members(other._members) {}
Reflection& Reflection::operator=(Reflection other) { ::swap(*this, other); return *this; }
void Reflection::swap(Reflection& other) /* noexcept */ {
	::swap(_class, other._class);
}
void swap(Reflection& a, Reflection& b) { a.swap(b); }

string Reflection::reprStruct(string name, Members members, bool json) const {
	std::stringstream out;
	if (json || Constants::jsonTrace) {
		out << "{\"class\":\"" << jsonEscape(name) << "\"";
		for (Members::const_iterator it = members.begin(); it != members.end(); ++it)
			out << ",\"" << it->first << "\":" << (this->*it->second.first)(true);
		out << "}";
	}
	else {
		out << kwrd(name) + punct("(");
		int i = 0;
		for (Members::const_iterator it = members.begin(); it != members.end(); ++it) {
			if (i++ != 0)
				out << punct(", ");
			if (Constants::kwargLogs)
				out << cmt(it->first) << cmt("=");
			out << (this->*it->second.first)(false);
		}
		out << punct(")");
	}
	return out.str();
}

void Reflection::reflectMember(ReprClosure reprClosure, const char *memberId, const void *memberPtr) {
	_members[memberId] = std::make_pair(reprClosure, memberPtr);
}

string Reflection::repr(bool json) const {
	return reprStruct(_class, _members, json);
}
