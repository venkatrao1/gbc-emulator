#pragma once

#include <algorithm>
#include <format>
#include <iterator>
#include <iostream>
#include <source_location>
#include <string>

namespace gb::logging {

enum class level : uint8_t {
	ERROR = 0,
	WARN = 1,
	INFO = 2,
	DEBUG = 3,
};

constexpr std::string_view to_string(const level lvl) {
	using enum level;
	switch(lvl) {
		case ERROR: return "ERROR";
		case WARN: return "WARN";
		case INFO: return "INFO";
		case DEBUG: return "DEBUG";
		default: throw std::logic_error(std::to_string(static_cast<uint8_t>(lvl)));
	}
}

constexpr static auto CURRENT_LOG_LEVEL = level::DEBUG;
const bool IS_TTY = true; // TODO: can detect if running w/ tty output, or add a flag

struct ANSIEscape {
	const std::string_view val;
};

std::ostream& operator<<(std::ostream& os, const ANSIEscape& esc);

namespace fg_colors {
constexpr ANSIEscape BLACK{"\x1B[0;30m"};
constexpr ANSIEscape RED{"\x1B[0;31m"};
constexpr ANSIEscape GREEN{"\x1B[0;32m"};
constexpr ANSIEscape YELLOW{"\x1B[0;33m"};
constexpr ANSIEscape BLUE{"\x1B[0;34m"};
constexpr ANSIEscape MAGENTA{"\x1B[0;35m"};
constexpr ANSIEscape CYAN{"\x1B[0;36m"};
constexpr ANSIEscape WHITE{"\x1B[0;37m"};
constexpr ANSIEscape NONE{"\x1B[0;0m"};
} // ns colors


// instead of printing full path to file, print just the last few directories.
// for example, this file becomes "include\gb/gb.h" on my machine (Windows paths are fun).
consteval std::string_view trim_src_path(const std::string_view file_orig) {
	constexpr std::string_view file{__FILE__};
	constexpr auto rootDir = file.substr(0, file.rfind("include"));
	static_assert(rootDir.size() < file.size());
	return file_orig.substr(std::mismatch(file_orig.begin(), file_orig.end(), rootDir.begin(), rootDir.end()).first - file_orig.begin());
}

struct SourceLocation {
	consteval SourceLocation(std::source_location loc = std::source_location::current()) : loc{loc}, trimmed_file{trim_src_path(loc.file_name())} {}
	std::source_location loc;
	std::string_view trimmed_file;
};


// we need the implicit conversion so we can automatically get std::source_location::current.
// we group it together with level because putting it after the variadic args is not easily possible.
struct LevelAndSourceLocation : SourceLocation {
	consteval LevelAndSourceLocation(level lvl, SourceLocation loc = std::source_location::current()) : lvl{lvl}, SourceLocation{loc} {}
	level lvl;
};

template<class... Args>
void log(const LevelAndSourceLocation lvl_sloc, std::format_string<Args...> fmt, Args&&... args) {
	if (lvl_sloc.lvl > CURRENT_LOG_LEVEL) return;

	using namespace fg_colors;
	// TODO add time?
	std::cout << GREEN << '[' << to_string(lvl_sloc.lvl) << "] " << CYAN << lvl_sloc.trimmed_file << ':' << lvl_sloc.loc.line() << ": " << NONE;
	std::format_to(std::ostream_iterator<char>(std::cout), std::move(fmt), std::forward<decltype(args)>(args)...);
	std::cout << '\n';
}

// prevent me from discarding the result of GB_exc
[[nodiscard]] std::runtime_error make_error(std::string str);

}

#define GB_log_error(fmt, ...) log(::gb::logging::level::ERROR, fmt, __VA_ARGS__)
#define GB_log_warn(fmt, ...) log(::gb::logging::level::WARN, fmt, __VA_ARGS__)
#define GB_log_info(fmt, ...) log(::gb::logging::level::INFO, fmt, __VA_ARGS__)
#define GB_log_debug(fmt, ...) log(::gb::logging::level::DEBUG, fmt, __VA_ARGS__)

#define GB_exc(fmt, ...) (::gb::logging::make_error(std::format("{}:{}: " fmt, ::gb::logging::trim_src_path(__FILE__), __LINE__, __VA_ARGS__)))