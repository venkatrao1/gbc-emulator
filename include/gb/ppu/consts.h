#pragma once

#include <gb/consts.h>

#include <array>
#include <cstdint>

namespace gb::ppu {

constexpr unsigned LCD_WIDTH = 160;
constexpr unsigned LCD_HEIGHT = 144;

constexpr unsigned VBLANK_LINES = 10;
constexpr unsigned MODE2_TCLKS = 80;
constexpr unsigned LINE_TCLKS = 456;

constexpr unsigned TILE_SZ = 8;
constexpr double FRAME_HZ = consts::TCLK_HZ / (LINE_TCLKS * (LCD_HEIGHT+VBLANK_LINES));

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

struct Gray {
	uint8_t raw{}; // 0 - 3
};

using Line = std::array<Gray, LCD_WIDTH>;
using Frame = std::array<Line, LCD_HEIGHT>;

}