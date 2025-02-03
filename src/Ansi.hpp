#pragma once /* Ansi.hpp */

#include <string>

// clang-format off
#define ANSI_CSI                 "\x1b\x5b"
#define ANSI_RGB                 "38;2"
#define ANSI_RGB_BG              "48;2"
#define ANSI_RST        ANSI_CSI      "m"
#define ANSI_BLACK      ANSI_CSI "30" "m"
#define ANSI_RED        ANSI_CSI "31" "m"
#define ANSI_GREEN      ANSI_CSI "32" "m"
#define ANSI_YELLOW     ANSI_CSI "33" "m"
#define ANSI_BLUE       ANSI_CSI "34" "m"
#define ANSI_MAGENTA    ANSI_CSI "35" "m"
#define ANSI_CYAN       ANSI_CSI "36" "m"
#define ANSI_WHITE      ANSI_CSI "37" "m"
#define ANSI_BLACK_BG   ANSI_CSI "40" "m"
#define ANSI_RED_BG     ANSI_CSI "41" "m"
#define ANSI_GREEN_BG   ANSI_CSI "42" "m"
#define ANSI_YELLOW_BG  ANSI_CSI "43" "m"
#define ANSI_BLUE_BG    ANSI_CSI "44" "m"
#define ANSI_MAGENTA_BG ANSI_CSI "45" "m"
#define ANSI_CYAN_BG    ANSI_CSI "46" "m"
#define ANSI_WHITE_BG   ANSI_CSI "47" "m"
// clang-format on

using std::string;

namespace ansi {
    bool noColor();
    string black(string s);
    string red(string s);
    string green(string s);
    string yellow(string s);
    string blue(string s);
    string magenta(string s);
    string cyan(string s);
    string white(string s);
    string blackBg(string s);
    string redBg(string s);
    string greenBg(string s);
    string yellowBg(string s);
    string blueBg(string s);
    string magentaBg(string s);
    string cyanBg(string s);
    string whiteBg(string s);
    string rgbP(string s, int r, int g, int b);
    string rgbBgP(string s, int r, int g, int b);
    string rgb(string s, const string &rgbSemicolon);
    string rgbBg(string s, const string &rgbSemicolon);
} // namespace ansi
