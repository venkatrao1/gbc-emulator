#include <gb/utils/print.h>

namespace gb {

std::ostream& operator<<(std::ostream& os, const print_hex p) {
	return (os << std::hex << p << std::dec);
}

}