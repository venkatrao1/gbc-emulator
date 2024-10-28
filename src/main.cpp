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
		emulator.run();
		GB_log_info("Exiting with code 0");
		return 0;
	} catch (const std::exception& e) {
		std::cerr << "Uncaught exception: " << e.what() << '\n';
		if(errno) {
			char strerror_buf[256]{};
			strerror_s(strerror_buf, 256, errno);
			std::cerr << "errno is \"" << strerror_buf << "\"\n";
		}
		return 1;
	}
}