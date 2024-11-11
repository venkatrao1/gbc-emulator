#include <gb/gb.h>
#include <gb/ui/ui.h>
#include <gb/utils/load_file.h>

#include <format>
#include <stdexcept>
#include <string_view>

namespace gb::ui::tui {

struct TUI : UI {
	static constexpr std::string_view name = "tui";

	TUI(int argc, const char* const argv[]) {
		if(argc < 4 || argc > 5) {
			const char* binary_name = argv[0] ? argv[0] : "<binary>";
			throw std::invalid_argument(std::format("Usage: {} tui <boot rom> <game rom> [save data]", binary_name));
		}
		auto bootrom = gb::load_file(argv[2]);
		auto cartridgerom = gb::load_file(argv[3]);
		std::optional<std::vector<uint8_t>> savedata;
		if(argc >= 5) savedata = gb::load_file(argv[4]);
		log_info("Loaded files");
		emulator.emplace(std::move(bootrom), std::move(cartridgerom), std::move(savedata));
	}

	void main_loop() override {
		emulator.value().run();
	}

	std::optional<gb::gameboy_emulator> emulator;
};

static auto registration [[maybe_unused]] = (UI::register_ui_type(TUI::name, [](int argc, const char* const argv[]){ return std::make_unique<TUI>(argc, argv); }), 0);

}