#pragma once /* Utils.hpp */

#include <vector>
#include <string>
#include <sstream>

#include "MacroMagic.h"

using std::vector;
using std::string;

namespace Utils {
	string parseArgs(int ac, char **av);

	template<typename T>
	static inline vector<T>& getTmpVec() {
		static vector<T> _;
		return _;
	}

	// TODO: @timo: improve repr to specify no color
	template <typename T>
	static inline string SSTR(T x) {
		std::ostringstream oss;
		oss << std::dec << x;
		return oss.str();
	}
}

#define TMP_VEC_PUSH_BACK(type, x) \
	Utils::getTmpVec<type>().push_back(x),

// Helper macro to create inplace vectors (i.e. passing as an argument), very useful sometimes
#define VEC(type, ...) \
	(Utils::getTmpVec<type>().clear(), FOR_EACH_ONE(TMP_VEC_PUSH_BACK, type, __VA_ARGS__) Utils::getTmpVec<type>())
