#include <string_view>
#include <vector>

#include <gb/cpu/cpu.h>
#include <gb/memory/mmu.h>
#include <gb/utils/log.h>

namespace gb
{

// the core of the GB emulator. contains all the pieces of the gameboy.
struct gameboy_emulator
{
	gameboy_emulator(std::vector<uint8_t> boot_rom, std::vector<uint8_t> cartridge_rom, std::optional<std::vector<uint8_t>> save_data)
		: mmu{std::move(boot_rom), std::move(cartridge_rom), std::move(save_data)}
	{
	}

	// the main loop. returns an exit code.
	void run() {
		GB_log_info("Starting gb");
		uint64_t cycle_count = 0;
		try {
			while(true) {
				cycle_count += cpu.fetch_execute();
			}
		} catch (...) {
			GB_log_error("Exception raised, CPU state:\n{}", cpu.dump_state());
			throw;
		}
	}

	memory::MMU mmu;
	cpu::CPU cpu{mmu};
};

}