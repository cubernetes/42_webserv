#include <cctype>
#include <cstddef>
#include <ostream>

#include "Constants.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

string HttpServer::getMimeType(const string &path) {
    size_t dotPos = path.find_last_of('.');
    log.trace() << "Doing a MIME type lookup for path: " << repr(path) << std::endl;
    if (dotPos == string::npos) {
        log.debug() << "Path " << repr(path)
                    << " doesn't have an extension, returning default MIME type "
                    << repr(Constants::defaultMimeType) << std::endl;
        return Constants::defaultMimeType; // file without an extension
    }

    string ext = Utils::strToLower(path.substr(dotPos + 1));
    MimeTypes::const_iterator it = _mimeTypes.find(ext);
    if (it != _mimeTypes.end()) {
        log.debug() << "Path " << repr(path) << " has registered extension " << repr(ext)
                    << ", returning MIME type " << repr(it->second) << std::endl;
        return it->second; // MIME type found
    }
    log.debug() << "Path " << repr(path) << " has extension " << repr(ext)
                << " which does not have an associated MIME type, returning default "
                   "MIME type "
                << repr(Constants::defaultMimeType) << std::endl;
    return Constants::defaultMimeType; // MIME type not found
}

void initMimeTypes(HttpServer::MimeTypes &mimeTypes) {
    Logger::lastInstance().debug()
        << "Initializing extension to MIME type mapping" << std::endl;

    // Web content
    mimeTypes["html"] = "text/html";
    mimeTypes["htm"] = "text/html";
    mimeTypes["css"] = "text/css";
    mimeTypes["js"] = "application/javascript";
    mimeTypes["xml"] = "application/xml";
    mimeTypes["json"] = "application/json";

    // Text files
    mimeTypes["txt"] = "text/plain";
    mimeTypes["csv"] = "text/csv";
    mimeTypes["md"] = "text/markdown";
    mimeTypes["sh"] = "text/x-shellscript";

    // Images
    mimeTypes["jpg"] = "image/jpeg";
    mimeTypes["jpeg"] = "image/jpeg";
    mimeTypes["png"] = "image/png";
    mimeTypes["gif"] = "image/gif";
    mimeTypes["svg"] = "image/svg+xml";
    mimeTypes["ico"] = "image/x-icon";
    mimeTypes["webp"] = "image/webp";

    // Documents
    mimeTypes["pdf"] = "application/pdf";
    mimeTypes["doc"] = "application/msword";
    mimeTypes["docx"] = "application/msword";
    mimeTypes["xls"] = "application/vnd.ms-excel";
    mimeTypes["xlsx"] = "application/vnd.ms-excel";
    mimeTypes["zip"] = "application/zip";

    // Multimedia
    mimeTypes["mp3"] = "audio/mpeg";
    mimeTypes["mp4"] = "video/mp4";
    mimeTypes["webm"] = "video/webm";

    // ...
}
