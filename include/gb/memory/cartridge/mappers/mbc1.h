#pragma once

#include <gb/utils/log.h>
#include <gb/memory/memory_map.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include <span>

namespace gb::memory::mappers {

struct MBC1 {
	MBC1(std::span<const uint8_t> romIn, std::optional<std::span<const uint8_t>> save_data): rom{romIn.begin(), romIn.end()} {
		const auto ram_size = [ram_size_raw = rom[memory::addrs::RAM_SIZE]](){
			switch(ram_size_raw) {
				case 0: return 0;
				case 2: return 1;
				case 3: return 4;
			}
			throw_exc("Unrecognized RAM size value {}", ram_size_raw);
		}();
		ram.resize(ram_size * BANK_SIZE);
		if(save_data.has_value()) {
			if(save_data->size() != ram.size()) throw_exc();
			std::copy(save_data->begin(), save_data->end(), ram.begin());
		}
	}
	
	uint8_t read(uint16_t addr) const {
		if(addr < 0x4000) { // ROM Bank 0
			unsigned bank = bank_mode_select ? bank_select_hi : 0;
			auto idx = (bank << 19) | addr;
			return rom[idx & (rom.size() - 1)];
		} else if(addr < 0x8000) { // ROM Bank 1
			unsigned idx = (addr & 0x3FFF) | (static_cast<unsigned>(bank_select_lo) << 14) | (static_cast<unsigned>(bank_select_hi) << 19);
			return rom[idx & (rom.size() - 1)];
		} else if(addr >= addrs::CARTRIDGE_RAM_BEGIN && addr < addrs::CARTRIDGE_RAM_END) {
			if(ram.empty()) {
				log_warn("Read from non-existent ram address {:#04x}", addr);
				return 0xFF;
			}
			if(!ram_enabled) return 0xFF;
			return get_ram(addr);
		}
		throw_exc("Invalid read from MBC1 addr {:#06x}", addr);
	}

	void write(uint16_t addr, uint8_t data) {
		if(addr >= addrs::CARTRIDGE_RAM_BEGIN && addr < addrs::CARTRIDGE_RAM_END) {
			if(ram.empty()) {
				log_warn("Write of {:#04x} to non-existent ram address {:#04x}", data, addr);
				return;
			}
			if(ram_enabled) {
				get_ram(addr) = data;
			}
			return;
		}
		if(addr < 0x8000) {
			if(addr < 0x2000) { // ram enable register
				ram_enabled = ((data & 0xF) == 0xA);
			} else if (addr < 0x4000) { // ROM bank number
				bank_select_lo = (data & 0b11111);
				if(bank_select_lo == 0) bank_select_lo = 1;
			} else if (addr < 0x6000) { // RAM / high ROM bank select
				bank_select_hi = (data & 0b11);
			} else if (addr < 0x8000) { // bank mode select
				bank_mode_select = data & 1;
			}
			log_debug("Wrote {:#04x} to MBC1 address {:#06x}, state:\n{}", data, addr, dump_state());
			return;
		}
		throw_exc("Invalid write of {:#04x} to address {:#06x}", data, addr);
	}

	auto dump_save_data() const { return std::nullopt; }

	std::string dump_state() const {
		return std::format(
			"bank_select_hi: {}, bank_select_lo: {}, bank_mode_select: {}, ram_enabled: {}",
			bank_select_hi, bank_select_lo, bank_mode_select, ram_enabled
		);
	}

private:
	const uint8_t& get_ram(uint16_t addr) const {
		unsigned bank = bank_mode_select ? bank_select_hi : 0;
		unsigned idx = (bank << 13) | (addr & 0x1FFF);
		return ram[idx & (ram.size() - 1)];
	}
	uint8_t& get_ram(uint16_t addr) { return const_cast<uint8_t&>(std::as_const(*this).get_ram(addr)); }

	constexpr static unsigned BANK_SIZE = 8'192;

	uint8_t bank_select_hi{0}; // 2 bit
	uint8_t bank_select_lo{1}; // 5 bit - set to 1 if 0 written
	bool bank_mode_select{0};
	bool ram_enabled{false};

	const std::vector<uint8_t> rom;
	std::vector<uint8_t> ram;
};

}