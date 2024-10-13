#pragma once

#include <gb/memory/memory_map.h>
#include <gb/memory/cartridge/mappers/no_mapper.h>

#include <cstdint>
#include <optional>
#include <vector>
#include <string>


namespace gb::memory {

// cartridge is effectively a variant of a bunch of mappers.
// the Mapper API is approximately as follows:

// constructor(std::vector<uint8_t> rom, optional<vector<uint8_t>> save_data)

// read/write:
// read(Addr) -> uint8_t
// write(Addr) -> void

// save data:
// dump_state() -> optional(vector<uint8_t>)
// loading state is done via the constructor.

class Cartridge {

public: 
	// TODO save data.
	Cartridge(std::vector<uint8_t> rom, std::optional<std::vector<uint8_t>> save_data);

	// for GB
	uint8_t read(uint16_t addr) const { return mapper.read(addr); };
	void write(uint16_t addr, uint8_t data) { mapper.write(addr, data); };
	// TODO: nice operator[] proxy?

	// for external (not by the emulated CPU) use
	std::string title() const {
		std::string ret;
		for(uint16_t addr = addrs::TITLE_BEGIN; addr <= addrs::TITLE_END; ++addr) {
			const char c = read(addr);
			if(c == '\0') break;
			ret.push_back(c);
		}
		return ret;
	}

	uint8_t version() const {
		return read(addrs::ROM_VERSION);
	}

private:
	mappers::NoMapper mapper;
};

}