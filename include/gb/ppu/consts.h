#pragma once

#include <gb/consts.h>

#include <array>
#include <cstdint>

namespace gb::ppu {

constexpr unsigned MODE2_TCLKS = 80;

constexpr unsigned TILE_SZ = 8;

// TODO: GBC
// struct Color {
// 	// produce a translated color from the original gameboy.
// 	static constexpr Color gb(uint8_t gray) {
// 		// TODO: make this green instead of gray?
// 		// 0 -> 0, 1 -> 11, 2 -> 20, 3 -> 31
// 		const uint16_t translate = (gray * 10) + (gray & 1);
// 		return {static_cast<uint16_t>(translate + (translate << 5) + (translate << 10))};
// 	}

// 	constexpr auto r() const { return raw & 0b11111; }
// 	constexpr auto g() const { return (raw >> 5) & 0b11111; }
// 	constexpr auto b() const { return (raw >> 10) & 0b11111; }

// 	// lowest 5 = red
// 	// next 5 = green
// 	// next 5 = blue
// 	// last bit is unused
// 	uint16_t raw{};
// };

constexpr uint8_t TRANSPARENT = 0;

struct Gray {
	uint8_t raw{}; // 0 - 3, 0 is lightest
};

using Line = std::array<Gray, LCD_WIDTH>;
using Frame = std::array<Line, LCD_HEIGHT>;

enum class Mode : uint8_t {
	HBLANK = 0,
	VBLANK = 1,
	RD_OAM = 2,
	DRAW = 3,
};

struct oam_entry {
	uint8_t y_plus_16;
	uint8_t x_plus_8;
	uint8_t tile_idx;
	uint8_t flags;
};

constexpr auto NUM_OAM_ENTRIES = (memory::addrs::OAM_END - memory::addrs::OAM_BEGIN) / sizeof(oam_entry);

}