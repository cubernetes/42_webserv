#pragma once

#include <string>
#include <utility>
#include <map>

using std::string;
using std::map;
using std::pair;

class Reflection;

typedef string (Reflection::*t_repr_closure)(bool json) const;
typedef pair<t_repr_closure, const void*> t_member;
typedef map<const char*, t_member> t_members;

class Reflection {
public:
	// due to -Weffc++
	virtual ~Reflection() {}
	Reflection();
	Reflection(const Reflection& other);
	Reflection& operator=(Reflection other);
	void swap(Reflection& other);

	// set by the REFLECT macro
	const char *_class;
	string get_class(const Reflection& v) { return v._class; }

	// populated by the REFELECT macro
	t_members _members;

	// generate a string representation of the class from _members by iterating over
	// it and using _memberToString to serialize it member by member
	string repr(bool json = false) const;
protected:
	// is implemented automatically in the derived class by the REFLECT macro from macro_magic.h
	void reflect() {} // empty by default, in case you're inheritin from this class and do the reflection in another (often post-periori, i.e. for another class that does not have reflection) way

	// Each DECL macro from macro_magic.h will generate 2 functions with deterministic names.
	// One of them is a closure that calls ::repr() for the specific member.
	// The other one is also a closure that calls reflect_member() with the required parameters.
	// The automatically implemented reflect() function (which is also a closure, in a way) in the derived class will call those closures
	// which call reflect_member(), which in turn adds the repr closures to the internal map of member.
	void reflect_member(t_repr_closure repr_closure, const char *memberId, const void *memberPtr);

	// return the string representation of a member
	string _memberToStr(const t_member& member, bool json = false) const;

	// Convenience print functions
	void print_copy_assign_op(const string& repr_other) const;
	void print_copy_ctor(const string& repr_other, const string& repr_this) const;
	void print_default_ctor(const string& repr_this) const;
	void print_dtor(const string& repr_this) const;
	void print_swap_begin(const string& repr_this, const string& repr_other) const;
	void print_swap_end() const;
	string repr_struct(string name, t_members members, bool json = false) const;
};
void swap(Reflection& a, Reflection& b) /* noexcept */;

#include "macro_magic.h"
#include "repr.hpp"
