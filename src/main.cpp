#include <gb/gb.h>
#include <gb/utils/load_file.h>
#include <gb/utils/sdl_log.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>

#include <fstream>
#include <iostream>
#include <string.h>


int main(int argc, char* argv[]) {
	gb::log::init_sdl_logging();

	const char* binary_name = argv[0] ? argv[0] : "<binary>";
	if(argc != 3 && argc != 4) {
		std::cerr << "Usage: " << binary_name << " <boot rom> <game rom> [save data]\n";
		return 1;
	}

	if(const auto sdl_err = SDL_Init(SDL_INIT_VIDEO); sdl_err < 0) {
		std::cerr << "Could not initialize SDL: " << SDL_GetError() << '\n';
		return 2;
	}

	try {
		{ // simple white window, based on https://lazyfoo.net/tutorials/SDL/01_hello_SDL/index2.php
			SDL_Window* window = GB_SDL_checkptr(SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN ));
			SDL_Surface* screenSurface = GB_SDL_checkptr(SDL_GetWindowSurface( window ));
			SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0xFF, 0xFF, 0xFF ) );
			SDL_UpdateWindowSurface( window );
			SDL_Event e; bool quit = false; while( quit == false ){ while( SDL_PollEvent( &e ) ){ if( e.type == SDL_QUIT ) quit = true; } }
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