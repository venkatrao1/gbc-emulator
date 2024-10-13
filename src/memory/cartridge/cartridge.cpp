
#include <gb/memory/cartridge/cartridge.h>

namespace gb::memory {

// cartridge is effectively a variant of a bunch of mappers.

Cartridge::Cartridge(std::vector<uint8_t> rom, std::optional<std::vector<uint8_t>> save_data)
	: mapper{std::move(rom), std::move(save_data)}
{
	std::cout << "Loaded cartridge with title \"" << title() << "\", version " << version() << '\n';
}


// constructor(std::vector<uint8_t> rom, optional<vector<uint8_t>> save_data)

// read/write:
// read(Addr) -> uint16_t
// write(Addr) -> void

// save data:
// dump_state() -> optional(vector<uint8_t>)
// loading state is done via the constructor.
}