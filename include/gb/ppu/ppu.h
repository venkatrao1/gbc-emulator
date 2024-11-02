#pragma once

#include "consts.h"
#include <gb/memory/mmu.h>

#include <sstream>

namespace gb::ppu {

struct PPU {
	PPU(gb::memory::MMU& mmu) : mmu{mmu} {
		// starting state == end of vblank
		lcd_status() = 0b1000'0000 | static_cast<uint8_t>(Mode::VBLANK);
		lcd_cur_y() = LCD_HEIGHT + VBLANK_LINES - 1;
	}

	const Frame& cur_frame() const { return frame; }

	auto tclk_tick() {
		// TODO: copy objects into buffer at beginning of mode 2 and sort
		// TODO: dma during mode 3 causes big issues.
		throw std::exception("foo");
	}

	std::string dump_state() const {
		std::ostringstream ret;
		#define DUMP_DEC(func_name) ret << #func_name "[" << static_cast<uint64_t>(func_name()) << "] "
		#define DUMP_HEX(func_name) ret << std::format(#func_name "[{:#04x}] ", static_cast<uint64_t>(func_name()))
		#define DUMP_BIN(func_name) ret << std::format(#func_name "[{:#010b}] ", static_cast<uint64_t>(func_name()))
		DUMP_BIN(lcd_control);
		DUMP_BIN(lcd_status);
		DUMP_DEC(lcd_cur_y);
		ret << "line_clks[" << static_cast<uint64_t>(line_clks) << "] cur_x[" << static_cast<uint64_t>(cur_x) << "]\n";
		DUMP_DEC(lcd_cmp_y);
		DUMP_HEX(oam_dma);
		ret << '\n';
		DUMP_BIN(bg_palette_data);
		DUMP_BIN(obj_palette0_data);
		DUMP_BIN(obj_palette1_data);
		ret << '\n';
		DUMP_DEC(lcd_scroll_y);
		DUMP_DEC(lcd_scroll_x);
		DUMP_DEC(lcd_window_x);
		DUMP_DEC(lcd_window_y);
		#undef DUMP_HEX
		#undef DUMP_BIN
		return std::move(ret).str();
	}

private:
	enum class Mode : uint8_t {
		HBLANK = 0,
		VBLANK = 1,
		RD_OAM = 2,
		DRAW = 3,
	};

	// memory helpers - nice names + basic const correctness
	[[nodiscard]] const uint8_t& lcd_control() const { return mmu.get<memory::addrs::LCD_CONTROL>(); };
	[[nodiscard]] const uint8_t& lcd_status() const { return mmu.get<memory::addrs::LCD_STATUS>(); };
	[[nodiscard]] uint8_t& lcd_status() { return mmu.get<memory::addrs::LCD_STATUS>(); };
	[[nodiscard]] const uint8_t& lcd_scroll_y() const { return mmu.get<memory::addrs::LCD_SCROLL_Y>(); };
	[[nodiscard]] const uint8_t& lcd_scroll_x() const { return mmu.get<memory::addrs::LCD_SCROLL_X>(); };
	[[nodiscard]] const uint8_t& lcd_cur_y() const { return mmu.get<memory::addrs::LCD_CUR_Y>(); }
	[[nodiscard]] uint8_t& lcd_cur_y() { return mmu.get<memory::addrs::LCD_CUR_Y>(); }
	[[nodiscard]] const uint8_t& lcd_cmp_y() const { return mmu.get<memory::addrs::LCD_CMP_Y>(); };
	[[nodiscard]] const uint8_t& oam_dma() const { return mmu.get<memory::addrs::OAM_DMA>(); };
	[[nodiscard]] const uint8_t& bg_palette_data() const { return mmu.get<memory::addrs::BG_PALETTE_DATA>(); };
	[[nodiscard]] const uint8_t& obj_palette0_data() const { return mmu.get<memory::addrs::OBJ_PALETTE0_DATA>(); }
	[[nodiscard]] const uint8_t& obj_palette1_data() const { return mmu.get<memory::addrs::OBJ_PALETTE1_DATA>(); };
	[[nodiscard]] const uint8_t& lcd_window_y() const { return mmu.get<memory::addrs::LCD_WINDOW_Y>(); };
	[[nodiscard]] const uint8_t& lcd_window_x() const { return mmu.get<memory::addrs::LCD_WINDOW_X>(); };

	// starting state == end of vblank
	uint16_t line_clks{LINE_TCLKS - 1}; // each tclk, counts up [0, LINE_TCLKS)
	uint16_t cur_x{LCD_WIDTH-1}; // cur pixel actually being drawn 
	gb::memory::MMU& mmu;
	Frame frame{};
};

}