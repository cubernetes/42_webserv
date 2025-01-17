// https://no-color.org/
// NO_COLOR=1 ./webserv -> no color output

#include <string>
#include <sstream>
#include <cstdlib>

#include "Ansi.hpp"

using std::string;

static string itoa(int n) {
	std::ostringstream oss;
	oss << n;
	return oss.str();
}

bool ansi::no_color() {
	const char *no_color = std::getenv("NO_COLOR");
	if (no_color == NULL)
		return false;
	else if (*no_color == '\0')
		return false;
	return true;
}

string ansi::black(string s) {
	if (no_color())
		return s;
	return ANSI_BLACK + s + ANSI_RST;
}

string ansi::red(string s) {
	if (no_color())
		return s;
	return ANSI_RED + s + ANSI_RST;
}

string ansi::green(string s) {
	if (no_color())
		return s;
	return ANSI_GREEN + s + ANSI_RST;
}

string ansi::yellow(string s) {
	if (no_color())
		return s;
	return ANSI_YELLOW + s + ANSI_RST;
}

string ansi::blue(string s) {
	if (no_color())
		return s;
	return ANSI_BLUE + s + ANSI_RST;
}

string ansi::magenta(string s) {
	if (no_color())
		return s;
	return ANSI_MAGENTA + s + ANSI_RST;
}

string ansi::cyan(string s) {
	if (no_color())
		return s;
	return ANSI_CYAN + s + ANSI_RST;
}

string ansi::white(string s) {
	if (no_color())
		return s;
	return ANSI_WHITE + s + ANSI_RST;
}

string ansi::black_bg(string s) {
	if (no_color())
		return s;
	return ANSI_BLACK_BG + s + ANSI_RST;
}

string ansi::red_bg(string s) {
	if (no_color())
		return s;
	return ANSI_RED_BG + s + ANSI_RST;
}

string ansi::green_bg(string s) {
	if (no_color())
		return s;
	return ANSI_GREEN_BG + s + ANSI_RST;
}

string ansi::yellow_bg(string s) {
	if (no_color())
		return s;
	return ANSI_YELLOW_BG + s + ANSI_RST;
}

string ansi::blue_bg(string s) {
	if (no_color())
		return s;
	return ANSI_BLUE_BG + s + ANSI_RST;
}

string ansi::magenta_bg(string s) {
	if (no_color())
		return s;
	return ANSI_MAGENTA_BG + s + ANSI_RST;
}

string ansi::cyan_bg(string s) {
	if (no_color())
		return s;
	return ANSI_CYAN_BG + s + ANSI_RST;
}

string ansi::white_bg(string s) {
	if (no_color())
		return s;
	return ANSI_WHITE_BG + s + ANSI_RST;
}

string ansi::rgb_p(string s, int r, int g, int b) {
	if (no_color())
		return s;
	return ANSI_CSI ANSI_RGB ";" + itoa(r) + ";" + itoa(g) + ";" + itoa(b) + "m" + s + ANSI_RST;
}

string ansi::rgb(string s, const string& rgb_semicolon) {
	if (no_color())
		return s;
	return ANSI_CSI ANSI_RGB ";" + rgb_semicolon + "m" + s + ANSI_RST;
}

string ansi::rgb_bg_p(string s, int r, int g, int b) {
	if (no_color())
		return s;
	return ANSI_CSI ANSI_RGB_BG ";" + itoa(r) + ";" + itoa(g) + ";" + itoa(b) + "m" + s + ANSI_RST;
}

string ansi::rgb_bg(string s, const string& rgb_semicolon) {
	if (no_color())
		return s;
	return ANSI_CSI ANSI_RGB_BG ";" + rgb_semicolon + "m" + s + ANSI_RST;
}
