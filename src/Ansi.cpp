// https://no-color.org/
// NO_COLOR=1 ./webserv -> no color output

#include <string>
#include <sstream>
#include <cstdlib>

#include "Ansi.hpp"

using std::string;

static inline string itoa(int n) {
	std::ostringstream oss;
	oss << n;
	return oss.str();
}

bool ansi::noColor() {
	const char *noColor = std::getenv("NO_COLOR");
	if (noColor == NULL)
		return false;
	else if (noColor[0] == '\0' || (noColor[0] == '0' && noColor[1] == '\0'))
		return false;
	return true;
}

string ansi::black(string s) {
	if (noColor())
		return s;
	return ANSI_BLACK + s + ANSI_RST;
}

string ansi::red(string s) {
	if (noColor())
		return s;
	return ANSI_RED + s + ANSI_RST;
}

string ansi::green(string s) {
	if (noColor())
		return s;
	return ANSI_GREEN + s + ANSI_RST;
}

string ansi::yellow(string s) {
	if (noColor())
		return s;
	return ANSI_YELLOW + s + ANSI_RST;
}

string ansi::blue(string s) {
	if (noColor())
		return s;
	return ANSI_BLUE + s + ANSI_RST;
}

string ansi::magenta(string s) {
	if (noColor())
		return s;
	return ANSI_MAGENTA + s + ANSI_RST;
}

string ansi::cyan(string s) {
	if (noColor())
		return s;
	return ANSI_CYAN + s + ANSI_RST;
}

string ansi::white(string s) {
	if (noColor())
		return s;
	return ANSI_WHITE + s + ANSI_RST;
}

string ansi::blackBg(string s) {
	if (noColor())
		return s;
	return ANSI_BLACK_BG + s + ANSI_RST;
}

string ansi::redBg(string s) {
	if (noColor())
		return s;
	return ANSI_RED_BG + s + ANSI_RST;
}

string ansi::greenBg(string s) {
	if (noColor())
		return s;
	return ANSI_GREEN_BG + s + ANSI_RST;
}

string ansi::yellowBg(string s) {
	if (noColor())
		return s;
	return ANSI_YELLOW_BG + s + ANSI_RST;
}

string ansi::blueBg(string s) {
	if (noColor())
		return s;
	return ANSI_BLUE_BG + s + ANSI_RST;
}

string ansi::magentaBg(string s) {
	if (noColor())
		return s;
	return ANSI_MAGENTA_BG + s + ANSI_RST;
}

string ansi::cyanBg(string s) {
	if (noColor())
		return s;
	return ANSI_CYAN_BG + s + ANSI_RST;
}

string ansi::whiteBg(string s) {
	if (noColor())
		return s;
	return ANSI_WHITE_BG + s + ANSI_RST;
}

string ansi::rgbP(string s, int r, int g, int b) {
	if (noColor())
		return s;
	return ANSI_CSI ANSI_RGB ";" + itoa(r) + ";" + itoa(g) + ";" + itoa(b) + "m" + s + ANSI_RST;
}

string ansi::rgb(string s, const string& rgbSemicolon) {
	if (noColor())
		return s;
	return ANSI_CSI ANSI_RGB ";" + rgbSemicolon + "m" + s + ANSI_RST;
}

string ansi::rgbBgP(string s, int r, int g, int b) {
	if (noColor())
		return s;
	return ANSI_CSI ANSI_RGB_BG ";" + itoa(r) + ";" + itoa(g) + ";" + itoa(b) + "m" + s + ANSI_RST;
}

string ansi::rgbBg(string s, const string& rgbSemicolon) {
	if (noColor())
		return s;
	return ANSI_CSI ANSI_RGB_BG ";" + rgbSemicolon + "m" + s + ANSI_RST;
}
