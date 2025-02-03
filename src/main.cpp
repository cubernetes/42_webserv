#include <cstdlib>
#include <exception>
#include <iostream>

#include "Ansi.hpp"
#include "Constants.hpp"
#include "Exceptions.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Options.hpp"

int main(int ac, char **av) {
    try {
        Options options(ac, av);
        if (options.printHelp || options.printVersion) {
            std::cout << (options.printHelp ? Constants::helpText : "webserv version " + Constants::webservVersion)
                      << std::endl;
            return EXIT_SUCCESS;
        }
        Logger log(std::cerr, options.logLevel);
        HttpServer server(options.configPath, log, options.onlyCheckConfig);
        server.run();
        return EXIT_SUCCESS;
    } catch (const OnlyCheckConfigException &exception) {
        return EXIT_SUCCESS;
    } catch (const std::exception &exception) {
        std::cout << Logger::fatalPrefix << exception.what() << std::endl;
        return EXIT_FAILURE;
    }
}
