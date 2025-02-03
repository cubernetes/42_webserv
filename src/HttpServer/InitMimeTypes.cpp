#include <algorithm>
#include <cctype>
#include <cstddef>

#include "Constants.hpp"
#include "HttpServer.hpp"

string HttpServer::getMimeType(const string &path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == string::npos)
        return Constants::defaultMimeType; // file without an extension

    string ext = path.substr(dotPos + 1);
    std::transform(
        ext.begin(), ext.end(), ext.begin(),
        static_cast<int (*)(int)>(
            ::tolower)); // a malicious MIME type could contain negative chars, leading to
                         // undefined behaviour with
                         // ::tolower, see
                         // https://stackoverflow.com/questions/5270780/what-does-the-mean-in-tolower
    MimeTypes::const_iterator it = _mimeTypes.find(ext);
    if (it != _mimeTypes.end())
        return it->second;             // MIME type found
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
