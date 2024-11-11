#pragma once

#include <GB/utils/log.h>

#include <cstdint>
#include <stdexcept>
#include <string>

namespace gb::memory::utils {

// conversion functions from values stored in rom/ram.

constexpr size_t get_rom_size(uint8_t data) {
	if(data > 8) throw_exc("rom size specifier too large: {:#x}", data);
	return 32'768ULL << data;
}

// # of 8kb ram banks, excluding MBC2 which is weird.
constexpr size_t get_ram_banks(uint8_t data) {
	switch(data) {
		case 0: return 0;
		case 2: return 1;
		case 3: return 4;
		case 4: return 16;
		case 5: return 8;
		default: throw_exc("invalid ram size specifier: {:#x}", data);
	}
}



}