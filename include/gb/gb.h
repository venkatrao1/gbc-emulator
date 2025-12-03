#include <string_view>
#include <vector>

#include <gb/cpu/cpu.h>
#include <gb/memory/mmu.h>
#include <gb/memory/serial.h>
#include <gb/ppu/ppu.h>
#include <gb/utils/log.h>

namespace gb
{

// the core of the GB emulator. contains all the pieces of the gameboy.
struct gameboy_emulator : SerialIO
{
	gameboy_emulator(std::vector<uint8_t> boot_rom, std::vector<uint8_t> cartridge_rom, std::optional<std::vector<uint8_t>> save_data)
		: mmu{std::move(boot_rom), std::move(cartridge_rom), std::move(save_data), joypad}
	{
	}

	void run_frame() {
		try {
			// run for 1 frame - wait for vblank to end, then wait for vblank to begin again.
			bool vblank_finished = false;
			while(!(vblank_finished && ppu.mode() == ppu::Mode::VBLANK)) {
				if(ppu.mode() != ppu::Mode::VBLANK) vblank_finished = true;
				const auto cpu_mclks = cpu.fetch_execute();
				const auto old_mclks = total_mclks;
				total_mclks += cpu_mclks;
				mmu.handle_timers(old_mclks, total_mclks);
				const auto cpu_tclks = cpu_mclks * 4; // TODO not true for CGB
				total_tclks += cpu_tclks;
				for(int i = 0; i<cpu_tclks; i++) ppu.tclk_tick();
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

	void press(joypad::joypad_bits pressed) {
		const auto joypad_flags = mmu.get<memory::addrs::JOYPAD>();
		const bool read_dpad = (joypad_flags >> 4) & 1;
		const bool read_buttons = (joypad_flags >> 5) & 1;
		const auto old_bits = joypad.read_nybble(read_buttons, read_dpad);
		joypad.press(pressed);
		if(const auto new_bits = joypad.read_nybble(read_buttons, read_dpad); new_bits != old_bits) {
			mmu.request_interrupt(gb::memory::interrupt_bits::JOYPAD);
			log_debug("Requested joypad interrupt");
		}
	}

	void release(joypad::joypad_bits released) {
		joypad.release(released);
	}

	// for debug
	const joypad::Joypad& get_joypad() const { return joypad; }

	// external gameboy requests to shift out a byte, return byte from memory to shift in
	uint8_t handle_serial_transfer([[maybe_unused]] uint8_t value, [[maybe_unused]] uint32_t baud) final {
		throw_exc();
	}

private:
	joypad::Joypad joypad;
public:
	memory::MMU mmu;
	cpu::CPU cpu{mmu};
	ppu::PPU ppu{mmu};

	uint64_t total_mclks = 0;
	uint64_t total_tclks = 0;

};

}