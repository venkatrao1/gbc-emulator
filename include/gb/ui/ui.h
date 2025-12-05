#pragma once

#include <map>
#include <memory>
#include <functional>
#include <string>
#include <string_view>

namespace gb::ui {

// UI for the gb emulator
// there can be multiple implementations (GUI, TUI, headless)
struct UI {
	// @return exit code for program
	virtual int main_loop() = 0;
	virtual ~UI() = default;

	using CreationFunction = std::function<std::unique_ptr<UI>(int argc, const char* const argv[])>;

	static std::unique_ptr<UI> create(std::string_view name, int argc, const char* const argv[]);
	static void register_ui_type(std::string_view name, CreationFunction); // should be called at program init
	static std::map<std::string, CreationFunction, std::less<>>& get_creation_functions(); // construct on first use
};

}