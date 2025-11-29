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
	switch(lvl) {
		using enum level;
		case ERROR: return "ERROR";
		case WARN: return "WARN";
		case INFO: return "INFO";
		case DEBUG: return "DEBUG";
		default: throw std::logic_error(std::to_string(static_cast<uint8_t>(lvl)));
	}
}

constexpr static auto CURRENT_LOG_LEVEL = level::DEBUG;
extern const bool IS_TTY;

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
	return {std::mismatch(file_orig.begin(), file_orig.end(), rootDir.begin(), rootDir.end()).first, file_orig.end()};
}

struct SourceLocation {
	consteval SourceLocation(std::source_location loc = std::source_location::current()) : loc{loc}, trimmed_file{trim_src_path(loc.file_name())} {}
	std::source_location loc;
	std::string_view trimmed_file;
};

// we need the implicit conversion so we can automatically get std::source_location::current.
// we group it together with the format string because putting it after the variadic args is not easily possible.
template<class... Args>
struct FmtAndSourceLocation : SourceLocation {
	consteval FmtAndSourceLocation(const auto& str, std::source_location loc = std::source_location::current()) : fmt{str}, SourceLocation{loc} {}
	constexpr FmtAndSourceLocation(std::format_string<Args...> fmt, SourceLocation loc) : fmt{fmt}, SourceLocation{loc} {}
	std::format_string<Args...> fmt;
};

template<class... Args>
void log(const level lvl, FmtAndSourceLocation<std::type_identity_t<Args>...> fmt_sloc, Args&&... args) {
	if (lvl > CURRENT_LOG_LEVEL) return;

	using namespace fg_colors;
	// TODO add time, use format
	std::cout << GREEN << '[' << to_string(lvl) << "] " << CYAN << fmt_sloc.trimmed_file << ':' << fmt_sloc.loc.line() << ": " << NONE;
	std::format_to(std::ostream_iterator<char>(std::cout), fmt_sloc.fmt, std::forward<decltype(args)>(args)...);
	std::cout << '\n';
}

}

namespace gb {

template<class... Args>
void log_info(logging::FmtAndSourceLocation<std::type_identity_t<Args>...> fmt_sloc, Args&&... args) {
	logging::log(logging::level::INFO, fmt_sloc, std::forward<Args>(args)...);
}

template<class... Args>
void log_error(logging::FmtAndSourceLocation<std::type_identity_t<Args>...> fmt_sloc, Args&&... args) {
	logging::log(logging::level::ERROR, fmt_sloc, std::forward<Args>(args)...);
}

template<class... Args>
void log_warn(logging::FmtAndSourceLocation<std::type_identity_t<Args>...> fmt_sloc, Args&&... args) {
	logging::log(logging::level::WARN, fmt_sloc, std::forward<Args>(args)...);
}

template<class... Args>
void log_debug(logging::FmtAndSourceLocation<std::type_identity_t<Args>...> fmt_sloc, Args&&... args) {
	logging::log(logging::level::DEBUG, fmt_sloc, std::forward<Args>(args)...);
}

template<class... Args>
[[noreturn]] void throw_exc(logging::FmtAndSourceLocation<std::type_identity_t<Args>...> fmt_sloc, Args&&... args) {
	std::string msg;
	auto iter = std::back_inserter(msg);
	std::format_to(iter, "{}:{}: ", fmt_sloc.trimmed_file, fmt_sloc.loc.line());
 	std::format_to(iter, fmt_sloc.fmt, std::forward<Args>(args)...);
	throw std::runtime_error{msg};
}

[[noreturn]] void throw_exc(logging::FmtAndSourceLocation<> fmt_sloc = "");

}