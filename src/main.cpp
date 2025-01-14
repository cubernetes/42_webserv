#include <iostream>

#include "HttpServer.hpp"
#include "conf.hpp"
// #include "repr.hpp"
#include "Server.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <stdexcept>

using std::cout;

int main(int ac, char **av) {
	try {
		HttpServer server;

		string configPath = Utils::parseArgs(ac, av);
		if (!server.setup(parseConfig(readConfig(configPath)))) {
            Logger::logerror("Failed to setup server");
            return 1;
        }

		server.run();
		// return (int)server.exitStatus;
		return 0;
	} catch (const std::runtime_error& error) {
		Logger::logexception(error);
		return 1;
	}
}
