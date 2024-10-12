namespace gb {

// cartridge is a variant of a bunch of mappers.
// the Mapper API is approximately as follows:

// constructor(std::span<uint8_t> rom, optional<span<uint8_t>> save_data)

// read/write:
// read(Addr) -> uint16_t
// write(Addr) -> void

// save data:
// dump_state() -> optional(vector<uint8_t>)
// loading state is done via the constructor.

}