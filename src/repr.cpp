#include <iostream>
#include <string>

#include "repr.hpp"
#include "ansi.hpp"

using std::string;
using ansi::rgb;
using ansi::rgb_bg;

void repr_init() {
	if (!ansi::no_color())
		std::cout << ANSI_FG;
}

void repr_done() {
	if (!ansi::no_color())
		std::cout << ANSI_RST_FR << '\n';
}

string repr_clr::str(string s) {
	return rgb_bg(rgb(s, ANSI_STR), ANSI_FG);
}

string repr_clr::chr(string s) {
	return rgb_bg(rgb(s, ANSI_CHR), ANSI_FG);
}

string repr_clr::kwrd(string s) {
	return rgb_bg(rgb(s, ANSI_KWRD), ANSI_FG);
}

string repr_clr::punct(string s) {
	return rgb_bg(rgb(s, ANSI_PUNCT), ANSI_FG);
}

string repr_clr::func(string s) {
	return rgb_bg(rgb(s, ANSI_FUNC), ANSI_FG);
}

string repr_clr::num(string s) {
	return rgb_bg(rgb(s, ANSI_NUM), ANSI_FG);
}

string repr_clr::var(string s) {
	return rgb_bg(rgb(s, ANSI_VAR), ANSI_FG);
}

string repr_clr::cmt(string s) {
	return rgb_bg(rgb(s, ANSI_CMT), ANSI_FG);
}
