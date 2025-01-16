#include <sstream>
#include <iostream>
#include <string>
#include <map>
#include <utility>
#include <cstddef>
#include <algorithm>

#include "repr.hpp"
#include "Reflection.hpp"
#include "Logger.hpp"
#include "HttpServer.hpp"

using std::string;
using std::map;
using std::pair;
using std::cout;

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

string Reflection::repr(bool json) const {
	std::stringstream out;
	if (json || Constants::jsonTrace) {
		out << "{\"class\":\"" << jsonEscape(name) << "\"";
		for (t_members::const_iterator it = members.begin(); it != members.end(); ++it)
			out << ",\"" << it->first << "\":" << (this->*it->second.first)(true);
		out << "}";
	}
	else {
		out << kwrd(name) + punct("(");
		int i = 0;
		for (t_members::const_iterator it = members.begin(); it != members.end(); ++it) {
			if (i++ != 0)
				out << punct(", ");
			if (Constants::keywordTrace)
				out << cmt(it->first) << cmt("=");
			out << (this->*it->second.first)(false);
		}
		out << punct(")");
	}
	return out.str();
}

void Reflection::reflect_member(t_repr_closure repr_closure, const char *memberId, const void *memberPtr) {
	_members[memberId] = std::make_pair(repr_closure, memberPtr);
}

string Reflection::repr(bool json) const {
	return repr_struct(_class, _members, json);
}

void Reflection::print_copy_assign_op(const string& repr_other) const {
	if (Constants::jsonTrace)
		cout << "{\"event\":\"copy assignment operator\",\"other object\":" << repr_other << "}\n";
	else
		cout << kwrd(_class) + punct("& ") + kwrd(_class) + punct("::") + func("operator") + punct("=(") << repr_other << punct(")") + '\n';
}

void Reflection::print_copy_ctor(const string& repr_other, const string& repr_this) const {
	if (Constants::jsonTrace)
		cout << "{\"event\":\"copy constructor\",\"other object\":" << repr_other << ",\"this object\":" << repr_this << "}\n";
	else
		cout << kwrd(_class) + punct("(") << repr_other << punct(") -> ") << repr_this << '\n';
}

void Reflection::print_default_ctor(const string& repr_this) const {
	if (Constants::jsonTrace)
		cout << "{\"event\":\"default constructor\",\"this object\":" << repr_this << "}\n";
	else
		cout << kwrd(_class) + punct("() -> ") << repr_this << '\n';
}

void Reflection::print_dtor(const string& repr_this) const {
	if (Constants::jsonTrace)
		cout << "{\"event\":\"destructor\",\"this object\":" << repr_this << "}\n";
	else
		cout << punct("~") << repr_this << '\n';
}

void Reflection::print_swap_begin(const string& repr_this, const string& repr_other) const {
	cout << cmt("<Swapping " + string(_class) + " *this:") + '\n';
	cout << repr_this << '\n';
	cout << cmt("with the following" + string(_class) + "object:") + '\n';
	cout << repr_other << '\n';
}

void Reflection::print_swap_end() const {
	cout << cmt(string(_class) + " swap done>") + '\n';
}
