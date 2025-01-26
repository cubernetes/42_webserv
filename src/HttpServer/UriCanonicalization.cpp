#include "HttpServer.hpp"

string HttpServer::percentDecode(const string& str) {
	std::ostringstream oss;
	for (size_t i = 0; i < str.length(); ++i) {
		if (str[i] != '%') {
			oss << str[i];
			continue;
		}
		if (i + 2 >= str.length()) {
			oss << str[i];
			continue;
		}
		if (!Utils::isHexDigitNoCase(str[i + 1]) || !Utils::isHexDigitNoCase(str[i + 2])) {
			oss << str[i];
			continue;
		}
		if (str[i + 1] == '0' && str[i + 2] == '0') {
			oss << str[i];
			continue;
		}
		oss << Utils::decodeTwoHexChars(str[i + 1], str[i + 2]);
		i += 2;
	}
	return oss.str();
}

string HttpServer::resolveDots(const string& str) {
	std::stringstream ss(str);
	string part;
	vector<string> parts;

	while (std::getline(ss, part, '/')) {
		if (part == "")
			continue;
		else if (part == ".")
			continue;
		else if (part == "..") {
			if (!parts.empty())
				parts.pop_back();
		} else
			parts.push_back(part);
	}
	if (part == "")
		parts.push_back(part);
	std::ostringstream oss;
	for (size_t i = 0; i < parts.size(); ++i) {
		oss << "/" << parts[i];
	}
	return oss.str();
}

string HttpServer::canonicalizePath(const string& path) {
	if (path.empty()) // see https://datatracker.ietf.org/doc/html/rfc2616#section-3.2.3
		return "/";
	string newPath = path;
	if (newPath[0] != '/')
		newPath = "/" + newPath;
	newPath = percentDecode(newPath);
	newPath = resolveDots(newPath);
	return newPath;
}
