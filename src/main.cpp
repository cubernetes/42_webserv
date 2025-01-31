#include <cstdlib>
#include <iostream>
#include <string>

#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

int main(int ac, char **av) {
  Logger log(std::cerr, std::getenv("DEBUG") ? Logger::DEBUG : Logger::INFO);
  try {
    std::string configPath = Utils::parseArgs(ac, av);
    HttpServer server(configPath, log);
    server.run();
    return EXIT_SUCCESS;
  } catch (const std::exception &exception) {
    log.fatal() << exception.what() << std::endl;
    return EXIT_FAILURE;
  }
}
