#pragma once

#include <gb/ui/ui.h>

#include <sstream>
#include <stdexcept>

namespace gb::ui {

std::unique_ptr<UI> UI::create(const std::string_view name, int argc, const char* const argv[]){
	const auto& creation_fns = get_creation_functions();
	const auto iter = creation_fns.find(name);
	if(iter == creation_fns.end()) {
		std::ostringstream msg;
		msg << "unrecognized UI type. registered UIs: [";
		for(const auto& [registered_name, _] : creation_fns) {
			msg << registered_name << ", ";
		}
		if(!creation_fns.empty()) msg.seekp(-2, std::ios::cur); // remove traililng comma
		msg << ']';
		throw std::invalid_argument(std::move(msg).str());
	}
	return iter->second(argc, argv);
}

void UI::register_ui_type(std::string_view name, CreationFunction fn) {
	get_creation_functions().emplace(name, std::move(fn));
}

std::map<std::string, UI::CreationFunction, std::less<>>& UI::get_creation_functions() {
	static std::map<std::string, UI::CreationFunction, std::less<>> functions;
	return functions;
}

}