#include <iostream>

#include "Config.hpp"
#include "HttpServer.hpp"
#include "conf.hpp"
#include "repr.hpp"
#include "Server.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <stdexcept>

using std::cout;

int main(int ac, char **av) {
	try {
		string configPath = Utils::parseArgs(ac, av);
		HttpServer server(configPath);
		if (!server.setup(config)) {
            Logger::logerror("Failed to setup server");
            return 1;
        }

		server.run();
		return (int)webserv.exitStatus;
	} catch (const std::runtime_error& error) {
		Logger::logexception(error);
		return 1;
	}
}
