#include <iostream>
#include <stdexcept>

#include "HttpServer.hpp"
#include "Server.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "conf.hpp"

using std::cout;

int main(int ac, char **av) {
	Server s;
	try {
		HttpServer server;
		cout << server << '\n';

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
