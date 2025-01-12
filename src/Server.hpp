#pragma once /* Server.hpp */

#include <string> /* std::string */
#include <iostream> /* std::ostream */

#include "repr.hpp" /* repr<T> */
#include "helper.hpp"
#include "Config.hpp"
#include "HttpServer.hpp"
#include "Reflection.hpp"
#include "conf.hpp"

using std::string;
using std::ostream;

class Server : public Reflection {
public:
	~Server();
	Server();
	Server(const string& confPath);
	Server(const Server& other);
	Server& operator=(Server);
	void swap(Server& other);
	operator string() const;

	void serve();

public:
	REFLECT(
		"Server",
		DECL(unsigned int, exitStatus),
		DECL(const string, rawConfig),
		DECL(t_config, config), // maybe private, since non-const?
		DECL(unsigned int, _id), // should be private
		DECL(HttpServer, _http), // should be private
	)
private:
	static unsigned int _idCntr;
};

template <typename T> struct repr_wrapper;
template <> struct repr_wrapper<Server> { static inline string repr(const Server& value, bool json = false) { return value.repr(json); } };
void swap(Server&, Server&) /* noexcept */;
ostream& operator<<(ostream&, const Server&);
