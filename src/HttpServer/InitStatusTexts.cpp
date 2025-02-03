#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Repr.hpp"

string HttpServer::statusTextFromCode(int statusCode) {
    string statusText;
    if (_statusTexts.find(statusCode) == _statusTexts.end())
        statusText = "Unknown Status Code";
    else
        statusText = _statusTexts[statusCode];
    Logger::lastInstance().debug() << "Mapping status code " << repr(statusCode)
                                   << " to status text " << repr(statusText) << std::endl;
    return statusText;
}

void initStatusTexts(HttpServer::StatusTexts &statusTexts) {
    Logger::lastInstance().debug()
        << "Initializing status code to status text mapping" << std::endl;
    statusTexts[100] = "Continue";
    statusTexts[101] = "Switching Protocols";
    statusTexts[102] = "Processing";
    statusTexts[103] = "Early Hints";
    statusTexts[200] = "OK";
    statusTexts[201] = "Created";
    statusTexts[202] = "Accepted";
    statusTexts[203] = "Non-Authoritative Information";
    statusTexts[204] = "No Content";
    statusTexts[205] = "Reset Content";
    statusTexts[206] = "Partial Content";
    statusTexts[207] = "Multi-Status";
    statusTexts[208] = "Already Reported";
    statusTexts[226] = "IM Used";
    statusTexts[300] = "Multiple Choices";
    statusTexts[301] = "Moved Permanently";
    statusTexts[302] = "Found";
    statusTexts[303] = "See Other";
    statusTexts[304] = "Not Modified";
    statusTexts[305] = "Use Proxy";
    statusTexts[306] = "Switch Proxy";
    statusTexts[307] = "Temporary Redirect";
    statusTexts[308] = "Permanent Redirect";
    statusTexts[404] = "error on Wikimedia";
    statusTexts[400] = "Bad Request";
    statusTexts[401] = "Unauthorized";
    statusTexts[402] = "Payment Required";
    statusTexts[403] = "Forbidden";
    statusTexts[404] = "Not Found";
    statusTexts[405] = "Method Not Allowed";
    statusTexts[406] = "Not Acceptable";
    statusTexts[407] = "Proxy Authentication Required";
    statusTexts[408] = "Request Timeout";
    statusTexts[409] = "Conflict";
    statusTexts[410] = "Gone";
    statusTexts[411] = "Length Required";
    statusTexts[412] = "Precondition Failed";
    statusTexts[413] = "Payload Too Large";
    statusTexts[414] = "URI Too Long";
    statusTexts[415] = "Unsupported Media Type";
    statusTexts[416] = "Range Not Satisfiable";
    statusTexts[417] = "Expectation Failed";
    statusTexts[418] = "I'm a teapot";
    statusTexts[421] = "Misdirected Request";
    statusTexts[422] = "Unprocessable Content";
    statusTexts[423] = "Locked";
    statusTexts[424] = "Failed Dependency";
    statusTexts[425] = "Too Early";
    statusTexts[426] = "Upgrade Required";
    statusTexts[428] = "Precondition Required";
    statusTexts[429] = "Too Many Requests";
    statusTexts[431] = "Request Header Fields Too Large";
    statusTexts[451] = "Unavailable For Legal Reasons";
    statusTexts[500] = "Internal Server Error";
    statusTexts[501] = "Not Implemented";
    statusTexts[502] = "Bad Gateway";
    statusTexts[503] = "Service Unavailable";
    statusTexts[504] = "Gateway Timeout";
    statusTexts[505] = "HTTP Version Not Supported";
    statusTexts[506] = "Variant Also Negotiates";
    statusTexts[507] = "Insufficient Storage";
    statusTexts[508] = "Loop Detected";
    statusTexts[510] = "Not Extended";
    statusTexts[511] = "Network Authentication Required";
}
