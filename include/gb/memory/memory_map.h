#pragma once

#include <cstdint>

namespace gb::memory::addrs {

// note - this file uses [begin, end], not [begin, end)

// broad categories
constexpr uint16_t CARTRIDGE_ROM_BEGIN{0x0000}, CARTRIDGE_ROM_END{0x7FFF};
constexpr uint16_t VRAM_BEGIN{0x8000}, VRAM_END{0x9FFF};
constexpr uint16_t CARTRIDGE_RAM_BEGIN{0xA000}, CARTRIDGE_RAM_END{0xBFFF};
constexpr uint16_t WORK_RAM_BEGIN{0xC000}, WORK_RAM_END{0xDFFF};
constexpr uint16_t ECHO_RAM_BEGIN{0xE000}, ECHO_RAM_END{0xFDFF};
constexpr uint16_t OAM_BEGIN{0xFE00}, OAM_END{0xFE9F};
constexpr uint16_t ILLEGAL_MEM_BEGIN{0xFEA0}, ILLEGAL_MEM_END{0xFEFF};
constexpr uint16_t IO_MMAP_BEGIN{0xFF00}, IO_MMAP_END{0xFF7F};
constexpr uint16_t HRAM_BEGIN{0xFF80}, HRAM_END{0xFFFE};
constexpr uint16_t IE{0xFFFF};

// within cartridge
constexpr uint16_t TITLE_BEGIN{0x0134}, TITLE_END{0x0142};
constexpr uint16_t CGB_FLAG{0x0143};
constexpr uint16_t CARTRIDGE_TYPE{0x0147};
constexpr uint16_t ROM_SIZE{0x0148};
constexpr uint16_t RAM_SIZE{0x0149};
constexpr uint16_t ROM_VERSION{0x014C};

// I/O
constexpr uint16_t JOYPAD{0xFF00};
constexpr uint16_t SERIAL_DATA{0xFF01}, SERIAL_CONTROL{0xFF02};
constexpr uint16_t DIVIDER{0xFF04};
constexpr uint16_t TIMER_COUNTER{0xFF05}, TIMER_MODULO{0xFF06}, TIMER_CONTROL{0xFF07};
// TODO: audio
constexpr uint16_t AUDIOS_BEGIN{0xFF10}, AUDIOS_END{0xFF26};
constexpr uint16_t WAVETABLE_RAM_BEGIN{0xFF30}, WAVETABLE_RAM_END{0xFF3F};
// LCD/Graphics
constexpr uint16_t LCD_CONTROL{0xFF40};
constexpr uint16_t LCD_STATUS{0xFF41};
constexpr uint16_t LCD_SCROLL_Y{0xFF42}, LCD_SCROLL_X{0xFF43};
constexpr uint16_t BG_PALETTE_DATA{0xFF47};
constexpr uint16_t OBJ_PALETTE0_DATA{0xFF48}, OBJ_PALETTE1_DATA{0xFF49};

constexpr uint16_t BOOT_ROM_SELECT{0xFF50};


constexpr uint16_t LCD_WINDOW_Y{0xFF4A}, LCD_WINDOW_X{0xFF4B};



}