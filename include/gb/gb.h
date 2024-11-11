#include <string_view>
#include <vector>

#include <gb/cpu/cpu.h>
#include <gb/memory/mmu.h>
#include <gb/ppu/ppu.h>
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

	void run_frame() {
		uint64_t cycle_count = 0;
		try {
			// run for 1 frame - wait for vblank to end, then wait for vblank to begin again.
			bool vblank_finished = false;
			while(!(vblank_finished && ppu.mode() == ppu::Mode::VBLANK)) {
				if(ppu.mode() != ppu::Mode::VBLANK) vblank_finished = true;
				const auto cpu_mclks = cpu.fetch_execute();
				for(int i = 0; i<cpu_mclks; i++) {
					for(int j = 0; j<4; j++) ppu.tclk_tick();
				}
				cycle_count += cpu_mclks;
			}
		} catch (...) {
			log_error("Exception raised, CPU state:\n{}\nPPU state:\n{}", cpu.dump_state(), ppu.dump_state());
			log_debug("frame:");
			char out[ppu::LCD_WIDTH]{};
			for(const auto& row : ppu.cur_frame()) {
				for(int col = 0; col < ppu::LCD_WIDTH; col++) {
					constexpr static char palette[4]{'@', 'O', 'o', ' '};
					out[col] = palette[row[col].raw];
				}
				log_debug("{}", std::string_view{out, ppu::LCD_WIDTH});
			}
			throw;
		}
	}

	// TODO: breakpoint for headless/tests
	void run() {
		log_info("Starting gb");
		while(true) run_frame();
	}

	memory::MMU mmu;
	cpu::CPU cpu{mmu};
	ppu::PPU ppu{mmu};
};

}