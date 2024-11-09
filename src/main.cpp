#include <gb/gb.h>
#include <gb/utils/load_file.h>
#include <gb/utils/sdl_log.h>

#include <SDL2/SDL.h>

#include <fstream>
#include <iostream>
#include <string.h>


int main(int argc, char* argv[]) {
	gb::log::init_sdl_logging();

	if(const auto sdl_err = SDL_Init(0); sdl_err < 0) {
		std::cerr << "Could not initialize SDL: " << SDL_GetError() << '\n';
		return 2;
	}

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
		SDL_Quit();
		return 0;
	} catch (const std::exception& e) {
		std::cerr << "Uncaught exception: " << e.what() << '\n';
		if(errno) {
			char strerror_buf[256]{};
			strerror_s(strerror_buf, sizeof(strerror_buf), errno);
			std::cerr << "errno is \"" << strerror_buf << "\"\n";
		}
		SDL_Quit();
		return 1;
	}
}