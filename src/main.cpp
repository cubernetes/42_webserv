#include <cstdlib>
#include <string>

#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

int main(int ac, char **av) {
  try {
    std::string configPath = Utils::parseArgs(ac, av);
    HttpServer server(configPath);
    server.run();
    return EXIT_SUCCESS;
  } catch (const std::exception &exception) {
    Logger::logFatal(exception.what());
    return EXIT_FAILURE;
  }
}
