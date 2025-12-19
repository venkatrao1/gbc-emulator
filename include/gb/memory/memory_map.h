#pragma once

#include <array>
#include <cstdint>
#include <format>
#include <string_view>

namespace gb::memory::addrs {

// note - this file uses [begin, end)

// broad categories
constexpr uint16_t CARTRIDGE_ROM_BEGIN{0x0000}, CARTRIDGE_ROM_END{0x8000};
constexpr uint16_t VRAM_BEGIN{0x8000}, VRAM_END{0xA000};
constexpr uint16_t CARTRIDGE_RAM_BEGIN{0xA000}, CARTRIDGE_RAM_END{0xC000};
constexpr uint16_t WORK_RAM_BEGIN{0xC000}, WORK_RAM_END{0xE000}; // DMG-only, CGB bank switches.
constexpr uint16_t ECHO_RAM_BEGIN{0xE000}, ECHO_RAM_END{0xFE00};
constexpr uint16_t OAM_BEGIN{0xFE00}, OAM_END{0xFEA0};
constexpr uint16_t ILLEGAL_MEM_BEGIN{0xFEA0}, ILLEGAL_MEM_END{0xFF00};
constexpr uint16_t IO_MMAP_BEGIN{0xFF00}, IO_MMAP_END{0xFF80};
constexpr uint16_t HRAM_BEGIN{0xFF80}, HRAM_END{0xFFFF};
constexpr uint16_t INTERRUPT_ENABLE{0xFFFF};

// within cartridge
constexpr uint16_t TITLE_BEGIN{0x0134}, TITLE_END{0x0143};
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
constexpr uint16_t INTERRUPT_FLAG{0xFF0F};
// Audio
constexpr uint16_t AUDIOS_BEGIN{0xFF10}, AUDIOS_END{0xFF40}; // all reads/writes here passed through to APU
// LCD/Graphics
constexpr uint16_t LCDS_BEGIN{0xFF40}, LCDS_END{0xFF4C};
constexpr uint16_t LCD_CONTROL{0xFF40};
constexpr uint16_t LCD_STATUS{0xFF41};
constexpr uint16_t LCD_SCROLL_Y{0xFF42}, LCD_SCROLL_X{0xFF43};
constexpr uint16_t LCD_CUR_Y{0xFF44}, LCD_CMP_Y{0xFF45};
constexpr uint16_t OAM_DMA{0xFF46};
constexpr uint16_t BG_PALETTE_DATA{0xFF47};
constexpr uint16_t OBJ_PALETTE0_DATA{0xFF48}, OBJ_PALETTE1_DATA{0xFF49};
constexpr uint16_t LCD_WINDOW_Y{0xFF4A}, LCD_WINDOW_X{0xFF4B};
// CGB registers
constexpr uint16_t KEY0{0xFF4C}, KEY1{0xFF4D};
constexpr uint16_t PCM12{0xFF76}, PCM34{0xFF77};

constexpr uint16_t BOOT_ROM_SELECT{0xFF50};
constexpr uint16_t BOOT_ROM_BEGIN{0x0000}, BOOT_ROM_END{0x0100}; // at least true for DMG

}

namespace gb::memory {

enum class interrupt_bits : uint8_t {
    VBLANK = 0,
    LCD = 1,
    TIMER = 2,
    SERIAL = 3,
    JOYPAD = 4
};

enum class serial_control_bits : uint8_t {
    CLOCK_SELECT = 0,
    CLOCK_SPEED = 1,
    ENABLE = 7
};

}

template<>
struct std::formatter<gb::memory::interrupt_bits> : std::formatter<std::string_view> {
	auto format(gb::memory::interrupt_bits i, auto& ctx) const {
		using namespace std::string_view_literals;
		constexpr static std::array names{
			"VBLANK"sv,
			"LCD"sv,
			"TIMER"sv,
			"SERIAL"sv,
			"JOYPAD"sv,
		};
		const auto raw = static_cast<uint8_t>(i);
		const auto name = raw < names.size() ? names[raw] : "UNKNOWN"sv;
		return std::formatter<std::string_view>::format(name, ctx);
	}
};