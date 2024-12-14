#include <iostream>

#include "Config.hpp"
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
		// Server webserv(config);

		// webserv.serve();
		// return webserv.exitStatus;
	} catch (const std::runtime_error& error) {
		Logger::logexception(error);
		return 1;
	}
}
