#include <sstream>
#include <string>
#include <utility>

#include "Constants.hpp"
#include "Reflection.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

using std::string;
using std::swap;

Reflection::Reflection() : _class(), _members() {}
Reflection::Reflection(const Reflection &other) : _class(other._class), _members(other._members) {}
Reflection &Reflection::operator=(Reflection other) {
  ::swap(*this, other);
  return *this;
}
void Reflection::swap(Reflection &other) /* noexcept */ { ::swap(_class, other._class); }
void swap(Reflection &a, Reflection &b) { a.swap(b); }

string Reflection::reprStruct(string name, Members members, bool json) const {
  std::stringstream out;
  if (json || Constants::jsonTrace) {
    out << "{\"class\":\"" << Utils::jsonEscape(name) << "\"";
    for (Members::const_iterator it = members.begin(); it != members.end(); ++it)
      out << ",\"" << it->first << "\":" << (this->*it->second.first)(true);
    out << "}";
  } else {
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

string Reflection::repr(bool json) const { return reprStruct(_class, _members, json); }
