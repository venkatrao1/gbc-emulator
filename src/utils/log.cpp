#include <gb/utils/log.h>

namespace gb::log {

std::ostream& operator<<(std::ostream& os, const ANSIEscape& esc) {
	if(IS_TTY) {
		os << esc.val;
	}
	return os;
}

}