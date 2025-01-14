#pragma once

#include <string>
#include <utility>
#include <map>

#include "macro_magic.h"

using std::string;
using std::map;
using std::pair;

enum e_type {
	INT,
	INTP,
	UINT,
	UINTP,
	CHAR,
	CHARP,
	UCHAR,
	UCHARP,
	SHORT,
	SHORTP,
	LONG,
	LONGP,
	ULONG,
	ULONGP,
	LLONG,
	LLONGP,
	ULLONG,
	ULLONGP,
	STRING,
	STRINGP,
	BOOL,
	BOOLP,
	LOGGER,
	LOGGERP,
	HTTPSERVER,
	HTTPSERVERP,
	UNKNOWN
};
typedef enum e_type t_type;

class Reflection {
public:
	virtual ~Reflection() {}
protected:
	string _memberToStr(const pair<t_type, const void*>& member, bool json = false) const;
	void reflect_member(t_type type, const char *memberId, const void *memberPtr);
	virtual void reflect() = 0;
	void print_copy_assign_op(const string& repr_other) const;
	void print_copy_ctor(const string& repr_other, const string& repr_this) const;
	void print_default_ctor(const string& repr_this) const;
	void print_dtor(const string& repr_this) const;
	void print_swap_begin(const string& repr_this, const string& repr_other) const;
	void print_swap_end() const;
	static string jsonEscape(string s);
public:
	const char *_class;
	map<const char*, pair<t_type, const void*> > _members;
	string repr(bool json = false) const;
};
