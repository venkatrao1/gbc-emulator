#include <gb/utils/log.h>

namespace gb::log {

std::ostream& operator<<(std::ostream& os, const ANSIEscape& esc) {
	if(IS_TTY) {
		os << esc.val;
	}
	return os;
}

std::runtime_error make_error(std::string str) {
	return std::runtime_error{std::move(str)};
}

}