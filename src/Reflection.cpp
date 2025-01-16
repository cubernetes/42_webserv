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

void Reflection::reflect_member(t_type type, const char *memberId, const void *memberPtr) {
	_members[memberId] = std::make_pair(type, memberPtr);
}

string Reflection::_memberToStr(const pair<t_type, const void*>& member, bool json) const {
	std::ostringstream oss;
	switch (member.first) {
		case STRING:
			oss << ::repr(*(string *)member.second, json);
			break;
		case STRINGP:
			oss << ::repr(*(string **)member.second, json);
			break;
		case INT:
			oss << ::repr(*(int *)member.second, json);
			break;
		case INTP:
			oss << ::repr(*(int **)member.second, json);
			break;
		case CHAR:
			oss << ::repr(*(char *)member.second, json);
			break;
		case CHARP:
			oss << ::repr(*(char **)member.second, json);
			break;
		case UCHAR:
			oss << ::repr(*(unsigned char *)member.second, json);
			break;
		case UCHARP:
			oss << ::repr(*(unsigned char **)member.second, json);
			break;
		case UINT:
			oss << ::repr(*(unsigned int *)member.second, json);
			break;
		case UINTP:
			oss << ::repr(*(unsigned int **)member.second, json);
			break;
		case LONG:
			oss << ::repr(*(long *)member.second, json);
			break;
		case LONGP:
			oss << ::repr(*(long **)member.second, json);
			break;
		case ULONG:
			oss << ::repr(*(unsigned long *)member.second, json);
			break;
		case ULONGP:
			oss << ::repr(*(unsigned long **)member.second, json);
			break;
		case LLONG:
			oss << ::repr(*(long long *)member.second, json);
			break;
		case LLONGP:
			oss << ::repr(*(long long **)member.second, json);
			break;
		case ULLONG:
			oss << ::repr(*(unsigned long long *)member.second, json);
			break;
		case ULLONGP:
			oss << ::repr(*(unsigned long long **)member.second, json);
			break;
		case SHORT:
			oss << ::repr(*(short *)member.second, json);
			break;
		case SHORTP:
			oss << ::repr(*(short **)member.second, json);
			break;
		case BOOL:
			oss << ::repr(*(bool *)member.second, json);
			break;
		case BOOLP:
			oss << ::repr(*(bool **)member.second, json);
			break;
		case LOGGER:
			oss << ::repr(*(Logger *)member.second, json);
			break;
		case LOGGERP:
			oss << ::repr(*(Logger **)member.second, json);
			break;
		case HTTPSERVER:
			oss << ::repr(*(HttpServer *)member.second, json);
			break;
		case HTTPSERVERP:
			oss << ::repr(*(HttpServer **)member.second, json);
			break;
		default:
			oss << ::repr(member.second, json); // TODO: RAW PTR: TEST COLOR
	}
	return oss.str();
}

string Reflection::repr(bool json) const {
	std::stringstream out;
	if (json || Constants::jsonTrace) {
		out << "{\"class\":\"" << jsonEscape(_class) << "\"";
		for (map<const char*, pair<t_type, const void*> >::const_iterator it = _members.begin(); it != _members.end(); ++it)
			out << ",\"" << it->first << "\":" << _memberToStr(it->second, true);
		out << "}";
	}
	else {
		out << kwrd(_class) + punct("(");
		int i = 0;
		for (map<const char*, pair<t_type, const void*> >::const_iterator it = _members.begin(); it != _members.end(); ++it) {
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

static string _replace(string s, const string& search,
                          const string& replace) {
	std::size_t pos = 0;
    while ((pos = s.find(search, pos)) != string::npos) {
         s.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return s;
}

string Reflection::jsonEscape(string s) {
	return _replace(_replace(s, "\\", "\\\\"), "\"", "\\\"");
}
