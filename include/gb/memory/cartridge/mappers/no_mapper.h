#pragma once

#include <gb/utils/print.h>
#include <gb/memory/memory_map.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <format>
#include <vector>

namespace gb::memory::mappers {

struct NoMapper {
	constexpr static size_t ROM_SIZE = 32'768;

	NoMapper(std::vector<uint8_t> romIn, std::optional<std::vector<uint8_t>> save_data) {
		if(save_data.has_value() && save_data->size() > 0) {
			throw std::runtime_error("Received non-empty save data for NoMapper");
		}

		if(romIn.size() != 32'768) {
			throw std::runtime_error(std::format("Received non-32k rom for NoMapper: size {}", rom.size()));
		}
		
		std::copy(romIn.begin(), romIn.end(), rom.begin());
	}

	uint8_t read(uint16_t addr) const {
		if(addr < addrs::CARTRIDGE_ROM_BEGIN || addr > addrs::CARTRIDGE_ROM_END) {
			throw std::runtime_error(std::format("Invalid read from NoMapper addr {:#x}", addr));
		}

		return rom[addr];
	}

	void write(uint16_t addr, [[maybe_unused]] uint8_t data) {
		// ignore writes to ROM
		std::cout << "Warning: wrote to ROM address " << print_hex{addr} << ", ignoring\n";
	}

	std::array<std::uint8_t, ROM_SIZE> rom;
};

}