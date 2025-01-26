#pragma once /* CgiHandler.hpp */

#include <map>
#include <ostream>
#include <string>
#include "HttpServer.hpp"

using std::string;

class CgiHandler {
public:
	~CgiHandler();
	CgiHandler();
	CgiHandler(const string& extension, const string& program);
	CgiHandler(const CgiHandler& other);
	CgiHandler& operator=(CgiHandler);
	void swap(CgiHandler&); // copy swap idiom

	// string conversion
	operator string() const;

	const string& get_extension() const;
	const string& get_program() const;

	bool canHandle(const string& path) const;
	void execute(int clientSocket, const HttpServer::HttpRequest& request, 
			const LocationCtx& location);
private:
	string _extension;
	string _program;

	std::map<string, string> setupEnvironment(const HttpServer::HttpRequest& request, 
											const LocationCtx& location);
	void exportEnvironment(const std::map<string, string>& env);
};

void swap(CgiHandler&, CgiHandler&) /* noexcept */;
std::ostream& operator<<(std::ostream&, const CgiHandler&);
