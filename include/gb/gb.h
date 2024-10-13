#include <string_view>
#include <vector>

#include <gb/memory/mmu.h>

namespace gb
{

	// the core of the GB emulator. contains all the pieces of the gameboy.
	struct gameboy_emulator
	{
		gameboy_emulator(std::vector<uint8_t> boot_rom, std::vector<uint8_t> cartridge_rom, std::optional<std::vector<uint8_t>> save_data)
			: mmu{std::move(boot_rom), std::move(cartridge_rom), std::move(save_data)}
		{
		}

		// the main loop.
		int run() {
			std::cout << "Starting gb\n"; 
			return 0;
		}

		memory::MMU mmu;
	};

}