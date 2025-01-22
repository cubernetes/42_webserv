#include <stdexcept>
#include <string>

#include "Utils.hpp"
#include "Constants.hpp"
#include "Errors.hpp"

using std::string;

string Utils::parseArgs(int ac, char** av) {
	if (ac == 1)
		return Constants::defaultConfPath;
	else if (ac == 2) {
		if (av && av[0] && av[1])
			return av[1];
		throw std::runtime_error(Errors::DegenerateArgv(ac, av));
	}
	throw std::runtime_error(Errors::WrongArgs(ac));
}
