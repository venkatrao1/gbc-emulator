#pragma once

#include <algorithm>
#include <format>
#include <iterator>
#include <iostream>
#include <string>

namespace gb::log {

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

template<class... Args>
void log(const level lvl, const std::string_view file, const size_t line, std::format_string<Args...> fmt, Args&&... args) {
	if (lvl > CURRENT_LOG_LEVEL) return;

	using namespace fg_colors;
	// TODO add time
	std::cout << GREEN << '[' << to_string(lvl) << "] " << CYAN << file << ':' << line << ": " << NONE;
	std::format_to(std::ostream_iterator<char>(std::cout), std::move(fmt), std::forward<decltype(args)>(args)...);
	std::cout << '\n';
}

// prevent me from discarding the result of GB_exc
[[nodiscard]] std::runtime_error make_error(std::string str);

}

#define GB_log_error(fmt, ...) ::gb::log::log(::gb::log::level::ERROR, ::gb::log::trim_src_path(__FILE__), __LINE__, fmt, __VA_ARGS__)
#define GB_log_warn(fmt, ...) ::gb::log::log(::gb::log::level::WARN, ::gb::log::trim_src_path(__FILE__), __LINE__, fmt, __VA_ARGS__)
#define GB_log_info(fmt, ...) ::gb::log::log(::gb::log::level::INFO, ::gb::log::trim_src_path(__FILE__), __LINE__, fmt, __VA_ARGS__)
#define GB_log_debug(fmt, ...) ::gb::log::log(::gb::log::level::DEBUG, ::gb::log::trim_src_path(__FILE__), __LINE__, fmt, __VA_ARGS__)

#define GB_exc(fmt, ...) (::gb::log::make_error(std::format("{}:{}: " fmt, ::gb::log::trim_src_path(__FILE__), __LINE__, __VA_ARGS__)))