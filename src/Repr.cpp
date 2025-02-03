#include <iostream>
#include <string>

#include "Ansi.hpp"
#include "Repr.hpp"

using ansi::noColor;
using ansi::rgb;
using ansi::rgbBg;
using std::string;

void reprInit() {
    if (!noColor())
        std::cout << ANSI_FG;
}

void reprDone() {
    if (!noColor())
        std::cout << ANSI_RST << '\n';
}

string ReprClr::str(string s) { return rgbBg(rgb(s, ANSI_STR), ANSI_FG); }

string ReprClr::chr(string s) { return rgbBg(rgb(s, ANSI_CHR), ANSI_FG); }

string ReprClr::kwrd(string s) { return rgbBg(rgb(s, ANSI_KWRD), ANSI_FG); }

string ReprClr::punct(string s) { return rgbBg(rgb(s, ANSI_PUNCT), ANSI_FG); }

string ReprClr::func(string s) { return rgbBg(rgb(s, ANSI_FUNC), ANSI_FG); }

string ReprClr::num(string s) { return rgbBg(rgb(s, ANSI_NUM), ANSI_FG); }

string ReprClr::var(string s) { return rgbBg(rgb(s, ANSI_VAR), ANSI_FG); }

string ReprClr::cmt(string s) { return rgbBg(rgb(s, ANSI_CMT), ANSI_FG); }
