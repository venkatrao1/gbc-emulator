#pragma once

#include <gb/memory/cartridge/cartridge.h>
#include <gb/memory/memory_map.h>
#include <gb/memory/serial.h>
#include <gb/consts.h>
#include <gb/utils/log.h>
#include <gb/utils/bitops.h>
#include <gb/joypad.h>

#include <optional>
#include <span>

namespace gb::memory {

class MMU {
public:
	MMU(std::span<const uint8_t> boot_rom_in, std::span<const uint8_t> cartridge_rom, std::optional<std::span<const uint8_t>> save_data, joypad::Joypad& joypad)
		: cartridge(std::move(cartridge_rom), std::move(save_data)), boot_rom(get_boot_rom(boot_rom_in)), joypad(joypad)
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
		} else if (addr < AUDIOS_BEGIN) {
			const auto& mem = high_mem[addr - IO_MMAP_BEGIN];
			switch(addr) {
				case JOYPAD: {
					const auto lower_nybble = joypad.read_nybble(!get_bit(mem, 5),!get_bit(mem, 4));
					return ((mem | 0b1100'0000) & 0xF0) | lower_nybble;
				}
				case SERIAL_DATA:
					return mem;
				case SERIAL_CONTROL: 
					return (mem | 0b0111'1110); // TODO: on CGB bit 1 has function too
				case DIVIDER:
				case TIMER_COUNTER:
				case TIMER_MODULO:
					return mem;
				case TIMER_CONTROL:
					return mem | 0b1111'1000;
				case INTERRUPT_FLAG:
					return mem | 0b1110'0000;
			}
			log_warn("Read from disconnected address {:#x}", addr);
			return 0xFF;
		} else if (addr < AUDIOS_END) {
			throw_exc("Unimplemented: memory read from {:#x}", addr);
		} else if (addr < IO_MMAP_END) {
			const auto& mem = high_mem[addr - IO_MMAP_BEGIN];
			if(addr >= LCDS_BEGIN && addr < LCDS_END) return mem; // all locations readable, TODO populate in PPU
			throw_exc("Unimplemented: memory read from {:#x}", addr);
		} else {
			return high_mem[addr - IO_MMAP_BEGIN];
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
				case JOYPAD:
					mem = mask_combine(0b0011'0000, mem, data);
					return;
				case SERIAL_DATA:
					if(serial_bits_remaining) throw_exc();
					mem = data;
					return;
				case SERIAL_CONTROL:
					mem = data; // TODO: on CGB bit 1 has function too
					if((mem & 0x81) == 0x81) { // both low and high bits set, start transfer with internal clock
						serial_shift_in = serial_conn.get().handle_serial_transfer(get<SERIAL_DATA>(), static_cast<unsigned>(consts::TCLK_HZ / (4 *SERIAL_MCLKS_PER_BIT)));
						serial_bits_remaining = 8;
					}
					return;
				case DIVIDER:
					mem = 0;
					return;
				case TIMER_COUNTER: // TODO emulate weird timer behavior
				case TIMER_MODULO:
				case TIMER_CONTROL:
				case INTERRUPT_FLAG:
					mem = data;
					return;
			} else if(addr < AUDIOS_END) {
				// TODO: audio unimplemented - for now allow writes
				mem = data;
				return;
			} else if(addr < WAVETABLE_RAM_END) {
				mem = data;
				return;
			} else if(addr < LCDS_END) switch(addr) {
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
				case OAM_DMA:
					start_oam_dma(data);
					return;
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

	const auto& oam_view() const {
		return oam;
	}

	// request an interrupt
	void request_interrupt(interrupt_bits i) { get<addrs::INTERRUPT_FLAG>() |= (1 << static_cast<uint8_t>(i)); }

	void handle_timers(uint64_t old_mclks, uint64_t new_mclks) {
		// TODO: this should just be a tick_mclk function.
		using namespace addrs;
		// DIV ticks up every 64 mclks.
		get<DIVIDER>() += static_cast<uint8_t>((new_mclks / 64) - (old_mclks / 64));

		if(serial_bits_remaining) {
			const auto serial_clks = (new_mclks / SERIAL_MCLKS_PER_BIT) - (old_mclks / SERIAL_MCLKS_PER_BIT);
			const auto num_shifts = static_cast<uint8_t>(std::min<decltype(serial_clks)>(serial_bits_remaining, serial_clks));
			// log_debug("Shifting out {} bits, {} clks since last called, {} bits remaining", num_shifts, new_mclks-old_mclks, serial_bits_remaining);
			auto& sb = get<addrs::SERIAL_DATA>();
			sb = (sb << num_shifts) + (serial_shift_in >> (8 - num_shifts));
			serial_shift_in <<= num_shifts;
			serial_bits_remaining -= num_shifts;
			if(serial_bits_remaining == 0) {
				rst_bit(get<addrs::SERIAL_CONTROL>(), 7);
				request_interrupt(interrupt_bits::SERIAL);
			}
		}

		if(const auto timer_control = get<TIMER_CONTROL>(); timer_control & 0b100) { // timer enabled
			const auto tima_mclks_shift = 2 + 2*((timer_control-1)&3);
			auto& tima = get<TIMER_COUNTER>();
			const auto num_ticks = (new_mclks >> tima_mclks_shift) - (old_mclks >> tima_mclks_shift);
			for(unsigned i = 0; i<num_ticks; i++) {
				if(++tima == 0) {
					tima = get<TIMER_MODULO>();
					request_interrupt(interrupt_bits::TIMER);
				}
			}
		}
	}

	void connect_serial(SerialIO& conn) {
		serial_conn = conn;
	}

private:
	Cartridge cartridge;
	bool boot_rom_enabled{true};
	const std::array<uint8_t, 256> boot_rom;
	std::array<uint8_t, addrs::VRAM_END - addrs::VRAM_BEGIN> vram{};
	std::array<uint8_t, addrs::WORK_RAM_END - addrs::WORK_RAM_BEGIN> wram{};
	std::array<uint8_t, addrs::OAM_END - addrs::OAM_BEGIN> oam{};
	std::array<uint8_t, addrs::INTERRUPT_ENABLE - addrs::IO_MMAP_BEGIN + 1> high_mem{}; // io regs + hram + ie

	// TODO: if SB written during a transfer, we need a bit-level protocol as opposed to current byte-level
	std::reference_wrapper<SerialIO> serial_conn{SERIAL_DISCONNECTED};
	uint8_t serial_shift_in{};
	uint8_t serial_bits_remaining = 0;
	constexpr static auto SERIAL_MCLKS_PER_BIT = 128; // TODO: not const for CGB

	const joypad::Joypad& joypad;

	static std::array<uint8_t, 256> get_boot_rom(const std::span<const uint8_t> boot_rom_in) {
		if(boot_rom_in.size() != 256) throw_exc("Boot rom has unexpected size {}", boot_rom_in.size());
		std::array<uint8_t, 256> ret;
		std::copy(boot_rom_in.begin(), boot_rom_in.end(), ret.begin());
		return ret;
	}

	void start_oam_dma(const uint8_t data) {
		// TODO: oam is not instant.
		get<addrs::OAM_DMA>() = data;
		const uint16_t src_addr = data << 8;
		log_debug("OAM DMA with source {:#06x}", src_addr);
		auto cur_addr = src_addr;
		for(uint8_t& oam_byte : oam) {
			oam_byte = read(cur_addr);
			cur_addr++;
		}
	}

	// TODO: disable access to vram etc during different PPU phases?
};

}