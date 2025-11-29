#include <gb/utils/log.h>
#include <io.h>

namespace gb::logging {

const bool IS_TTY = _isatty(_fileno(stdout));

std::ostream& operator<<(std::ostream& os, const ANSIEscape& esc) {
	if(IS_TTY) {
		os << esc.val;
	}
	return os;
}

}

namespace gb {

void throw_exc(logging::FmtAndSourceLocation<> fmt_sloc) {
	throw_exc<>(fmt_sloc);
}

}