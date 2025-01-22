#include <iostream>
#include <stdexcept>

#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

using std::cout;

int main(int ac, char **av) {
	try {
		HttpServer server; // TODO: @discuss: consider initializing server with `const Config&' (see Server.hpp), since a server without a config is impossible

		string configPath = Utils::parseArgs(ac, av);
		if (!server.setup(parseConfig(readConfig(configPath)))) { // TODO: @discuss: when initializing using constructor instead, we would throw a runtime exception instead which would be caught below
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
