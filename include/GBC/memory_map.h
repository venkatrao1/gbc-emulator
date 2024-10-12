#include "types.h"

namespace gb {

// note - this file uses [begin, end], not [begin, end)

constexpr Addr CARTRIDGE_ROM_BEGIN{0x0000}, CARTRIDGE_ROM_END{0x7FFF};
constexpr Addr VRAM_BEGIN{0x8000}, VRAM_END{0x9FFFF};
constexpr Addr CARTRIDGE_RAM_BEGIN{0xA000}, CARTRIDGE_RAM_END{0xBFFF};
constexpr Addr WORK_RAM_BEGIN{0xC000}, WORK_RAM_END{0xDFFF};
constexpr Addr ECHO_RAM_BEGIN{0xE000}, ECHO_RAM_END{0xFDFF};
constexpr Addr OAM_BEGIN{0xFE00}, OAM_END{0xFE9F};
constexpr Addr ILLEGAL_MEM_BEGIN{0xFEA0}, ILLEGAL_MEM_END{0xFEFF};
constexpr Addr IO_MMAP_BEGIN{0xFF00}, IO_MMAP_END{0xFF7F};
constexpr Addr HRAM_BEGIN{0xFF80}, HRAM_END{0xFFFE};
constexpr Addr IE_REGISTER{0xFFFF};

}