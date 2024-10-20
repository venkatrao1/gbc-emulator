#include <iostream>
#include <gb/gb.h>
#include <gb/utils/load_file.h>
#include <string.h>

int main(int argc, char* argv[]) {
	try {
		const char* binary_name = argv[0] ? argv[0] : "<binary>"; 
		if(argc != 3 && argc != 4) {
			std::cerr << "Usage: " << binary_name << " <boot rom> <game rom> [save data]\n";
			return 1;
		}
		auto bootrom = gb::load_file(argv[1]);
		auto cartridgerom = gb::load_file(argv[2]);
		std::optional<std::vector<uint8_t>> savedata;
		if(argc >= 4) savedata = gb::load_file(argv[3]);
		GB_log_info("Loaded files");

		gb::gameboy_emulator emulator{std::move(bootrom), std::move(cartridgerom), std::move(savedata)};
		const auto gb_result = emulator.run();
		GB_log_info("Exiting with code {}", gb_result);
		return gb_result;
	} catch (const std::exception& e) {
		char strerror_buf[256]{};
		strerror_s(strerror_buf, 256, errno);
		std::cerr << "Uncaught exception:\nerrno is \"" << strerror_buf << "\"\nException: " << e.what() << '\n';
		return 1;
	}
}