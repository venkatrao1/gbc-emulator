#pragma once

#include <gb/memory/cartridge/cartridge.h>
#include <gb/memory/memory_map.h>
#include <gb/utils/log.h>

#include <optional>
#include <vector>
#include <span>

namespace gb::memory {

class MMU {
public:
	MMU(std::vector<uint8_t> boot_rom_in, std::vector<uint8_t> cartridge_rom, std::optional<std::vector<uint8_t>> save_data)
		: cartridge(std::move(cartridge_rom), std::move(save_data)), boot_rom(get_boot_rom(boot_rom_in)) 
	{}

	uint8_t read(const uint16_t addr) const {
		using namespace addrs;
		if(addr < CARTRIDGE_ROM_END) {
			if(addr < BOOT_ROM_END && get<BOOT_ROM_SELECT>() == 0) {
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

private:
	Cartridge cartridge;
	const std::array<uint8_t, 256> boot_rom;
	std::array<uint8_t, addrs::VRAM_END - addrs::VRAM_BEGIN> vram{};
	std::array<uint8_t, addrs::WORK_RAM_END - addrs::WORK_RAM_BEGIN> wram{};
	std::array<uint8_t, addrs::OAM_END - addrs::OAM_BEGIN> oam{};
	std::array<uint8_t, addrs::IE - addrs::IO_MMAP_BEGIN + 1> high_mem{}; // io regs + hram + ie

	static std::array<uint8_t, 256> get_boot_rom(const std::span<const uint8_t> boot_rom_in) {
		if(boot_rom_in.size() != 256) throw std::runtime_error("Boot rom has unexpected size " + std::to_string(boot_rom_in.size()));
		std::array<uint8_t, 256> ret;
		std::copy(boot_rom_in.begin(), boot_rom_in.end(), ret.begin());
		return ret;
	}
};

}