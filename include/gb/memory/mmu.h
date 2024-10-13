#pragma once

#include <gb/memory/cartridge/cartridge.h>

#include <optional>
#include <vector>


namespace gb::memory {

struct MMU {
	MMU(std::vector<uint8_t> boot_rom_in, std::vector<uint8_t> cartridge_rom, std::optional<std::vector<uint8_t>> save_data)
		: cartridge(std::move(cartridge_rom), std::move(save_data)) 
	{
		if(boot_rom_in.size() != 256) throw std::runtime_error("Boot rom has unexpected size " + std::to_string(boot_rom.size()));
		std::copy(boot_rom_in.begin(), boot_rom_in.end(), boot_rom.begin());
	}

	std::array<uint8_t, 256> boot_rom;
	Cartridge cartridge;
};

}