#pragma once

#include <iostream>

namespace gb {

struct print_hex {
	print_hex() = delete;
	print_hex(size_t in) : val{in} {}
	size_t val;
};

std::ostream& operator<<(std::ostream& os, const print_hex h);

}