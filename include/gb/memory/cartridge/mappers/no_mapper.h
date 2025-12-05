#pragma once

#include <gb/utils/log.h>
#include <gb/memory/memory_map.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace gb::memory::mappers {

struct NoMapper {
	constexpr static size_t ROM_SIZE = 32'768;

	NoMapper(std::vector<uint8_t> romIn, std::optional<std::vector<uint8_t>> save_data) {
		if(save_data.has_value() && save_data->size() > 0) {
			throw_exc("Received non-empty save data for NoMapper");
		}

		if(romIn.size() != 32'768) {
			throw_exc("Received non-32k rom for NoMapper: size {}", romIn.size());
		}

		std::copy(romIn.begin(), romIn.end(), rom.begin());
	}

	uint8_t read(uint16_t addr) const {
		if(addr < addrs::CARTRIDGE_ROM_BEGIN || addr > addrs::CARTRIDGE_ROM_END) {
			throw_exc("Invalid read from NoMapper addr {:#x}", addr);
		}

		return rom[addr];
	}

	void write(uint16_t addr, [[maybe_unused]] uint8_t data) {
		log_warn("Wrote to ROM address {:#x}, ignoring", addr);
	}

	std::array<std::uint8_t, ROM_SIZE> rom;
};

}