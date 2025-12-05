#include <gb/gb.h>
#include <gb/ui/ui.h>
#include <gb/utils/load_file.h>
#include <gb/memory/serial.h>

#include <format>
#include <stdexcept>
#include <string_view>

namespace gb::ui::blargg {

struct BlarggUI : UI, SerialIO {
	static constexpr std::string_view name = "blargg_harness";

	BlarggUI(int argc, const char* const argv[]) {
		if(argc < 4 || argc > 5) {
			const char* binary_name = argv[0] ? argv[0] : "<binary>";
			throw std::invalid_argument(std::format("Usage: {} blargg_harness <boot rom> <game rom> [save data]", binary_name));
		}
		auto bootrom = gb::load_file(argv[2]);
		auto cartridgerom = gb::load_file(argv[3]);
		std::optional<std::vector<uint8_t>> savedata;
		if(argc >= 5) savedata = gb::load_file(argv[4]);
		log_info("Loaded files");
		emulator.emplace(std::move(bootrom), std::move(cartridgerom), std::move(savedata));
		emulator->connect_serial(*this);
	}

	int main_loop() override {
		emulator.value().run();
	}

	std::string serial_buf;
	uint8_t handle_serial_transfer(uint8_t value, [[maybe_unused]] uint32_t baud) final {
		if(value == 0) throw_exc();
		if(value == '\n') {
			log_info("Test ROM wrote:\n{}", serial_buf);
			serial_buf.clear();
		} else {
			serial_buf.push_back(value);
		}

		return SERIAL_DISCONNECTED_VALUE;
	}

	std::optional<gb::gameboy_emulator> emulator;
};

static auto registration [[maybe_unused]] = (UI::register_ui_type(BlarggUI::name, [](int argc, const char* const argv[]){ return std::make_unique<BlarggUI>(argc, argv); }), 0);

}