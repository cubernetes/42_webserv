#include <stdexcept>
#include <string>

#include "Utils.hpp"
#include "Constants.hpp"
#include "Errors.hpp"

using std::string;

namespace Utils {
}

string Utils::parseArgs(int ac, char** av) {
	if (ac == 1)
		return Constants::defaultConfPath;
	else if (ac == 2)
		return av[1];
	throw std::runtime_error(Errors::WrongArgs(ac - 1));
}
