#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>

#include "HttpServer.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

string HttpServer::percentDecode(const string &str) {
    log.trace() << "Percent decoding string: " << repr(str) << std::endl;
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
        log.trace2() << "Found a valid percent token for decoding: " << num(string("%") + str[i + 1] + str[i + 2]) << std::endl;
        oss << Utils::decodeTwoHexChars(str[i + 1], str[i + 2]);
        i += 2;
    }
    log.trace() << "Percent decoded string is: " << repr(oss.str()) << std::endl;
    return oss.str();
}

string HttpServer::resolveDots(const string &str) {
    log.trace() << "Resolving dots, double dots, and slashes in path: " << repr(str) << std::endl;
    std::stringstream ss(str);
    string part;
    vector<string> parts;

    while (std::getline(ss, part, '/')) {
        if (part == "") {
            log.trace2() << "Found empty path segment, ignoring" << std::endl;
            continue;
        } else if (part == ".") {
            log.trace2() << "Found single dot in path, ignoring" << std::endl;
            continue;
        } else if (part == "..") {
            log.trace2() << "Found double dot in path, ignoring and trying to remove "
                            "previous path segment"
                         << std::endl;
            if (!parts.empty()) {
                parts.pop_back();
            } else {
                log.trace2() << "No previous path segment (at root), not going up" << std::endl;
            }
        } else {
            log.trace2() << "Adding regular path segment: " << repr(part) << std::endl;
            parts.push_back(part);
        }
    }
    if (part == "") {
        log.trace2() << "Path ends in slash, preserving this slash" << std::endl;
        parts.push_back(part);
    }
    std::ostringstream oss;
    log.trace2() << "Concatenating path segments " << repr(parts) << " back to a single string (leading empty segment is implicit)" << std::endl;
    for (size_t i = 0; i < parts.size(); ++i) {
        oss << "/" << parts[i];
    }
    log.trace() << "Resolved path is: " << repr(oss.str()) << std::endl;
    return oss.str();
}

string HttpServer::canonicalizePath(const string &path) {
    log.debug() << "Canonicalizing request path: " << repr(path) << std::endl;
    if (path.empty()) { // see https://datatracker.ietf.org/doc/html/rfc2616#section-3.2.3
        log.debug() << "It was empty, returning " << repr((char *)"/") << std::endl;
        return "/";
    }
    string newPath = path;
    if (newPath[0] != '/') {
        log.warn() << "Request URI " << repr(path)
                   << " does not start with a slash, although a relativeURI (or rather, "
                      "abs_path, https://www.rfc-editor.org/rfc/rfc2616#section-3.2.2) "
                      "was expected. Adding a slash in front as fallback"
                   << std::endl;
        newPath = "/" + newPath;
    }
    newPath = percentDecode(newPath);
    newPath = resolveDots(newPath);
    log.debug() << "Canonicalized path is: " << repr(newPath) << std::endl;
    return newPath;
}
