#include <gb/gb.h>
#include <gb/ui/ui.h>
#include <gb/utils/load_file.h>
#include <gb/memory/serial.h>

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

namespace gb::ui::mooneye {

struct MooneyeUI : UI, SerialIO {
	static constexpr std::string_view name = "mooneye_harness";

	MooneyeUI(int argc, const char* const argv[]) {
		if(argc < 4 || argc > 5) {
			const char* binary_name = argv[0] ? argv[0] : "<binary>";
			throw std::invalid_argument(std::format("Usage: {} mooneye_harness <boot rom> <game rom> [save data]", binary_name));
		}
		auto bootrom = gb::load_file(argv[2]);
		auto cartridgerom = gb::load_file(argv[3]);
		std::optional<std::vector<uint8_t>> savedata;
		if(argc >= 5) savedata = gb::load_file(argv[4]);
		log_info("Loaded files");
		emulator.emplace(std::move(bootrom), std::move(cartridgerom), std::move(savedata));
	}

	int main_loop() override {
		// TODO: timeout
		constexpr unsigned TIMEOUT_FRAMES = static_cast<unsigned>(60 * ppu::FRAME_HZ);
		for(unsigned cur_frames = 0; cur_frames < TIMEOUT_FRAMES; ++cur_frames) {
			emulator->run_frame();
			if(const auto status = validate_output(); status.has_value()) {
				log_info("Dumping state:\n{}", emulator->dump_state());
				log_info("Exiting with code {}", *status);
				return *status;
			}
		}
		log_error("Test did not complete after {} frames", TIMEOUT_FRAMES);
		log_error("Dumping state:\n{}", emulator->dump_state());
		return 1;
	}

	/// @return nullopt if not done, return code if done
	std::optional<int> validate_output() const {
		bool passed_starts_with_output = std::mismatch(TEST_PASSED.begin(), TEST_PASSED.end(), serial_output.begin(), serial_output.end()).second == serial_output.end();
		if(passed_starts_with_output && (serial_output.size() == TEST_PASSED.size())) {
			log_info("Test passed!");
			return 0;
		} else if (!passed_starts_with_output) {
			std::string output_str;
			for(auto b : serial_output) {
				std::format_to(std::back_inserter(output_str), "{}, ", b);
			}
			output_str.resize(output_str.size()-2);
			log_error("Test failed! Serial output: [{}]", output_str);
			return 1;
		}
		return std::nullopt;
	}

	uint8_t handle_serial_transfer(uint8_t value, [[maybe_unused]] uint32_t baud) final {
		serial_output.push_back(value);
		return SERIAL_DISCONNECTED_VALUE;
	}

	constexpr static std::array<uint8_t, 6> TEST_PASSED{3, 5, 8, 13, 21, 34};
	std::vector<uint8_t> serial_output;

	std::optional<gb::gameboy_emulator> emulator;
};

static auto registration [[maybe_unused]] = (UI::register_ui_type(MooneyeUI::name, [](int argc, const char* const argv[]){ return std::make_unique<MooneyeUI>(argc, argv); }), 0);

}