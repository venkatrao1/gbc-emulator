#pragma once

#include <gb/memory/cartridge/cartridge.h>
#include <gb/memory/memory_map.h>
#include <gb/utils/log.h>
#include <gb/utils/bitops.h>

#include <optional>
#include <vector>
#include <span>

namespace gb::memory {

class MMU {
public:
	MMU(std::vector<uint8_t> boot_rom_in, std::vector<uint8_t> cartridge_rom, std::optional<std::vector<uint8_t>> save_data)
		: cartridge(std::move(cartridge_rom), std::move(save_data)), boot_rom(get_boot_rom(boot_rom_in))
	{}

	// right now I'm throwing on behavior I never expect to see (illegal reads/writes)
	// TODO: these should have real behavior.

	// read, as if from the CPU.
	// other pieces of hardware may be able to read different addresses at different times.
	// (ex. PPU can read VRAM while drawing, cpu can't)
	// TODO: more realistic access control for memory
	uint8_t read(const uint16_t addr) const {
		using namespace addrs;
		if(addr < CARTRIDGE_ROM_END) {
			if(addr < BOOT_ROM_END && boot_rom_enabled) {
				return boot_rom[addr - BOOT_ROM_BEGIN];
			}
			return cartridge.read(addr);
		} else if (addr < VRAM_END) {
			return vram[addr - VRAM_BEGIN];
		} else if (addr < CARTRIDGE_RAM_END) {
			return cartridge.read(addr);
		} else if (addr < WORK_RAM_END) {
			return wram[addr - WORK_RAM_BEGIN];
		} else if (addr < ECHO_RAM_END) {
			return wram[addr - ECHO_RAM_BEGIN];
		} else if (addr < OAM_END) {
			return oam[addr - OAM_BEGIN];
		} else if (addr < ILLEGAL_MEM_END) {
			throw_exc("Illegal memory read from {:#x}", addr);
		} else if (addr < IO_MMAP_END) {
			const auto& mem = high_mem[addr - IO_MMAP_BEGIN];
			switch(addr) {
				case DIVIDER:
					return mem;
			}
			if(addr >= LCDS_BEGIN && addr < LCDS_END) return mem; // all locations readable, TODO populate in PPU
			throw_exc("Unimplemented: memory read from {:#x}", addr);
		} else if(addr < INTERRUPT_ENABLE) {
			return high_mem[addr - IO_MMAP_BEGIN];
		} else {
			return get<INTERRUPT_ENABLE>() | 0b1110'0000;
		}
	}

	// write, as if from the CPU (see note on read above).
	void write(const uint16_t addr, const uint8_t data) {
		using namespace addrs;
		if(addr < CARTRIDGE_ROM_END) {
			cartridge.write(addr, data);
		} else if (addr < VRAM_END) {
			vram[addr - VRAM_BEGIN] = data;
		} else if (addr < CARTRIDGE_RAM_END) {
			cartridge.write(addr, data);
		} else if (addr < WORK_RAM_END) {
			wram[addr - WORK_RAM_BEGIN] = data;
		} else if (addr < ECHO_RAM_END) {
			wram[addr - ECHO_RAM_BEGIN] = data;
		} else if (addr < OAM_END) {
			oam[addr - OAM_BEGIN] = data;
		} else if (addr < ILLEGAL_MEM_END) {
			log_warn("Illegal memory write to {:#06x}", addr);
			// TODO: doing nothing for now - if we have to implement reads revisit this
		} else if (addr < IO_MMAP_END) {
			if(addr >= 0xFF78) { // not mapped to any register
				log_warn("Write to {:#06x}, ignoring", addr);
				return;
			}
			auto& mem = high_mem[addr - IO_MMAP_BEGIN];
			if(addr < AUDIOS_BEGIN) switch(addr) {
				case SERIAL_DATA:
					log_warn("Writing serial data {:#04x} but it is unimplemented", data);
					mem = data;
					return;
				case SERIAL_CONTROL:
					if(data & (1 << static_cast<uint8_t>(serial_control_bits::ENABLE))) throw_exc("Serial unimplemented; control data {:#04x}", data);
					mem = data;
					return;
				case INTERRUPT_FLAG:
					mem = data;
					return;
			} else if(addr < AUDIOS_END) {
				// TODO: audio unimplemented - for now allow writes
				mem = data;
				return;
			} else if(addr >= LCDS_BEGIN && addr < LCDS_END) switch(addr) {
				// NOTE: only listing writable regs, anything else falls through
				case LCD_CONTROL:
					log_debug("LCDC {:08b}", data); // TODO remove
					mem = data;
					return;
				case LCD_STATUS:
					mem = mask_combine(0b0111'1000, mem, data);
					return;
				case LCD_SCROLL_Y:
				case LCD_SCROLL_X:
					mem = data;
					return;
				case LCD_CMP_Y:
					mem = data;
					return;
				case OAM_DMA: break; // TODO
				case BG_PALETTE_DATA:
				case OBJ_PALETTE0_DATA:
				case OBJ_PALETTE1_DATA:
				case LCD_WINDOW_Y:
				case LCD_WINDOW_X:
					mem = data;
					return;
			} else if (addr == BOOT_ROM_SELECT) {
				mem = data;
				if(data) boot_rom_enabled = false;
				return;
			}
			throw_exc("Unimplemented: memory write to {:#x}", addr);
		} else {
			high_mem[addr - IO_MMAP_BEGIN] = data;
		}
	}

	// to refer to an addr (probably an io reg) with only compile-time checking.
	template<uint16_t Addr>
	auto& get() {
		if constexpr (Addr >= addrs::IO_MMAP_BEGIN) {
			return high_mem[Addr - addrs::IO_MMAP_BEGIN];
		}
	}

	template<uint16_t Addr>
	const uint8_t& get() const {
		return const_cast<MMU*>(this)->get<Addr>();
	}

	// for PPU usage
	const uint8_t* vram_begin() const {
		return vram.data();
	}

	// request an interrupt
	void request_interrupt(interrupt_bits i) { get<addrs::INTERRUPT_FLAG>() |= (1 << static_cast<uint8_t>(i)); }

	void handle_timers(uint64_t old_mclks, uint64_t new_mclks) {
		using namespace addrs;
		// DIV ticks up every 64 mclks.
		if((new_mclks ^ old_mclks) & (~63)) get<DIVIDER>()++;
	}

private:
	Cartridge cartridge;
	bool boot_rom_enabled{true};
	const std::array<uint8_t, 256> boot_rom;
	std::array<uint8_t, addrs::VRAM_END - addrs::VRAM_BEGIN> vram{};
	std::array<uint8_t, addrs::WORK_RAM_END - addrs::WORK_RAM_BEGIN> wram{};
	std::array<uint8_t, addrs::OAM_END - addrs::OAM_BEGIN> oam{};
	std::array<uint8_t, addrs::INTERRUPT_ENABLE - addrs::IO_MMAP_BEGIN + 1> high_mem{}; // io regs + hram + ie

	static std::array<uint8_t, 256> get_boot_rom(const std::span<const uint8_t> boot_rom_in) {
		if(boot_rom_in.size() != 256) throw_exc("Boot rom has unexpected size {}", boot_rom_in.size());
		std::array<uint8_t, 256> ret;
		std::copy(boot_rom_in.begin(), boot_rom_in.end(), ret.begin());
		return ret;
	}

	// TODO: disable access to vram etc during different PPU phases?
};

}