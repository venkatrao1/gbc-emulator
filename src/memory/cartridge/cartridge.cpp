#include <gb/memory/memory_map.h>
#include <gb/memory/cartridge/cartridge.h>
#include <gb/memory/cartridge/mappers/no_mapper.h>
#include <gb/memory/cartridge/mappers/mbc1.h>

namespace gb::memory {

// Figure out which mapper should be used, and initialize it.
// Also checks some common info to all mappers (rom/ram size correct, etc)
Cartridge::mapper_variant_t init_mapper(std::span<const uint8_t> rom, std::optional<std::span<const uint8_t>> save_data) {
	if(rom.size() < 32'768) throw_exc("Rom size of {} bytes too small", rom.size());
	
	const auto rom_size_raw = rom[memory::addrs::ROM_SIZE];
	if(rom_size_raw > 0x08) throw_exc("Unrecognized ROM size value {}", rom_size_raw);
	const auto rom_size_bytes = 32'768 << rom_size_raw;
	if(rom.size() != rom_size_bytes) throw_exc("Expected ROM size {} based on header, actually {} bytes", rom_size_bytes, rom.size());
	
	if(save_data.has_value()) {
		const auto ram_size_raw = rom[memory::addrs::RAM_SIZE];
		const auto ram_banks = [ram_size_raw](){
			switch(ram_size_raw) {
				case 0: return 0;
				case 2: return 1;
				case 3: return 4;
				case 4: return 16;
				case 5: return 8;
			}
			throw_exc("Unrecognized RAM size value {}", ram_size_raw);
		}();
		const auto ram_size_bytes = ram_banks * 8'192; 
		if(save_data->size() != ram_size_bytes) throw_exc("Expected RAM size {} based on header, actually {} bytes", ram_size_bytes, save_data->size());
	}

	const auto cartridge_type = rom[memory::addrs::CARTRIDGE_TYPE];
	switch(cartridge_type){
		using namespace mappers;
		case 0x00: // ROM only
			return NoMapper{rom, save_data};
		case 0x01: // MBC1
		case 0x02: // MBC1+RAM
		case 0x03: // MBC1+RAM+battery
			return MBC1{rom, save_data};
	}
	throw_exc("Unrecognized cartridge type {:#04x}", cartridge_type);
}

Cartridge::Cartridge(std::span<const uint8_t> rom, std::optional<std::span<const uint8_t>> save_data): mapper_variant{init_mapper(rom, save_data)}
{
	log_info("Loaded cartridge with title \"{}\", version {}", title(), read(addrs::ROM_VERSION));
}


// constructor(std::vector<uint8_t> rom, optional<vector<uint8_t>> save_data)

// read/write:
// read(Addr) -> uint16_t
// write(Addr) -> void

// save data:
// dump_state() -> optional(vector<uint8_t>)
// loading state is done via the constructor.
}