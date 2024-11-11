#include <gb/utils/log.h>

namespace gb::logging {

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

namespace gb {

void throw_exc(logging::FmtAndSourceLocation<> fmt_sloc) {
	throw_exc<>(fmt_sloc);
}

}