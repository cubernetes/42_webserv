#include <iostream>

#include "Config.hpp"
#include "HttpServer.hpp"
#include "Server.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <stdexcept>

using std::cout;

int main(int ac, char **av) {
	try {
		string configPath = Utils::parseArgs(ac, av);
		Config config(configPath);
		cout << "Config is:\n";
		cout << config << '\n';
		HttpServer server;
		if (!server.setup(config)) {
            Logger::logerror("Failed to setup server");
            return 1;
        }
		server.run();
        return 0;
	} catch (const std::runtime_error& error) {
		Logger::logexception(error);
		return 1;
	}
}
