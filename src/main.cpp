#include <iostream>

#include "compiler.hpp"


int main(int argc, char* argv[]) {
	if (argc > 1) {
		std::cout << bsq::Compile(argv[1]) << std::endl;
		return 0;
	}

	std::cout << bsq::Compile("../tests/bubble.bas");

	/*const std::string prefix = "../tests/test";
	for (size_t i = 0; i <= 17; ++i) {
		auto filename = prefix + (i <= 9 ? "0" : "") + std::to_string(i) + ".bas";
		std::cout << filename << std::endl;
		std::cout << bsq::Compile(filename) << std::endl;
	}*/
}
