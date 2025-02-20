#pragma once

#include "consts.h"
#include <gb/memory/mmu.h>

#include <sstream>

namespace gb::ppu {

struct PPU {
	PPU(gb::memory::MMU& mmu) : mmu{mmu} {
		reset();
	}

	const Frame& cur_frame() const { return frame; }

	void reset() {
		log_debug("resetting PPU");
		// starting state == completely off, most things zeroed.
		// starting state == end of vblank
		// TODO: the real starting state should be on line 0
		was_last_off = true;
		stat_interrupt_wanted = false;
		frame = {};
		lcd_status() = 0b1000'0000 | static_cast<uint8_t>(Mode::HBLANK);
		lcd_cur_y() = 0;
		line_clks = LINE_TCLKS - 1; // TODO: not sure if this is right.
	}

	void tclk_tick() {
		// TODO: LCD should generate interrupts (only when on), LCD should be reset when turned off
		const auto cur_mode = mode();
		const auto LCDC = lcd_control();
		if(const bool lcd_enable = get_bit(LCDC, 7); !lcd_enable) {
			if(was_last_off) return; // don't continually reset
			if(cur_mode != Mode::VBLANK) throw_exc("turned off LCD while in mode {}", static_cast<int>(cur_mode));
			reset();
			return;
		}

		was_last_off = false;

		Mode next_mode = cur_mode;
		const auto EOL = line_clks == LINE_TCLKS - 1;

		if(cur_mode == Mode::DRAW) {
			// For now, rough approximation of PPU behavior.
			// will flesh out later.
			if(line_clks >= MODE2_TCLKS) {
				if(const unsigned cur_x = line_clks - MODE2_TCLKS; cur_x < LCD_WIDTH) {
					// draw a pixel - for now only bg, ignoring delay
					const bool enable_bg = LCDC & 1;
					if(enable_bg) {
						static_assert(0b1'0000 << 7 == 0x800);
						const uint8_t* bg_tiledata = mmu.vram_begin() + ((~LCDC & 0b1'0000) << 7); // 0x8800 if 0, 0x8000 if 1
						static_assert(0b1000 << 7 == 0x400);
						const uint8_t* bg_tilemap = mmu.vram_begin() + 0x1800 + ((LCDC & 0b1000) << 7); // 0x9800 if 0, 0x9C00 if 1
						const auto bg_x = static_cast<uint8_t>(lcd_scroll_x() + cur_x);
						const auto bg_y = static_cast<uint8_t>(lcd_scroll_y() + lcd_cur_y());
						const uint8_t tile_idx = bg_tilemap[(static_cast<uint16_t>(bg_y >> 3) << 5) | (bg_x >> 3)];
						const uint8_t* bg_tilerow = bg_tiledata + (((tile_idx * TILE_SZ) + (bg_y & 7)) * 2);
						const uint8_t palette_idx = ((bg_tilerow[0] >> (7-(bg_x & 7))) & 1) | (((bg_tilerow[1] >> (7-(bg_x & 7))) & 1) << 1);
						const uint8_t color = (bg_palette_data() >> (2 * palette_idx)) & 3;
						frame[lcd_cur_y()][cur_x] = {color};
					}
					if(cur_x == LCD_WIDTH - 1) next_mode = Mode::HBLANK;
				}
			}
		}

		if(EOL) {
			line_clks = 0;
			if(++lcd_cur_y() == LCD_HEIGHT + VBLANK_LINES) {
				lcd_cur_y() = 0;
			};
			if(lcd_cur_y() < LCD_HEIGHT) {
				next_mode = Mode::RD_OAM;
			} else {
				next_mode = Mode::VBLANK;
				if(cur_mode != Mode::VBLANK) mmu.request_interrupt(memory::interrupt_bits::VBLANK);
			}
		}
		else {
			++line_clks;
			if(line_clks == MODE2_TCLKS && cur_mode == Mode::RD_OAM) next_mode = Mode::DRAW;
			// (DRAW -> HBLANK) transition handled above.
		}

		// stat interrupt check
		bool next_stat_interrupt_wanted = false;
		if((next_mode != Mode::VBLANK) && get_bit(lcd_status(), static_cast<uint8_t>(next_mode) + 3)) {
			next_stat_interrupt_wanted = true;
		}
		const bool lyc_equals_ly = lcd_cmp_y() == lcd_cur_y();
		if(get_bit(lcd_status(), 6) && lyc_equals_ly) {
			next_stat_interrupt_wanted = true;
		}
		if(next_stat_interrupt_wanted && !stat_interrupt_wanted) {
			mmu.request_interrupt(memory::interrupt_bits::LCD);
		}
		stat_interrupt_wanted = next_stat_interrupt_wanted;

		lcd_status() = mask_combine<uint8_t>(0b0000'0111, lcd_status(), (lyc_equals_ly << 2) | static_cast<uint8_t>(next_mode));

		// TODO: copy objects into buffer at beginning of mode 2 and sort
		// TODO: dma during mode 3 causes big issues.
	}

	std::string dump_state() const {
		std::ostringstream ret;
		#define DUMP_DEC(func_name) ret << #func_name "[" << static_cast<uint64_t>(func_name()) << "] "
		#define DUMP_HEX(func_name) ret << std::format(#func_name "[{:#04x}] ", func_name())
		#define DUMP_BIN(func_name) ret << std::format(#func_name "[{:#010b}] ", func_name())
		DUMP_BIN(lcd_control);
		DUMP_BIN(lcd_status);
		DUMP_DEC(lcd_cur_y);
		ret << "line_clks[" << static_cast<uint64_t>(line_clks) << "] cur_x[" << line_clks - MODE2_TCLKS << "]\n";
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
		#undef DUMP_DEC
		#undef DUMP_HEX
		#undef DUMP_BIN
		return std::move(ret).str();
	}

	Mode mode() const {
		return static_cast<Mode>(lcd_status() & 3);
	}

private:

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
	uint16_t line_clks; // each tclk, counts up [0, LINE_TCLKS)
	// uint16_t cur_x{LCD_WIDTH-1}; // cur pixel actually being drawn
	bool was_last_off;
	bool stat_interrupt_wanted;
	// TODO: ppu only starts drawing a frame after it is enabled.
	gb::memory::MMU& mmu;
	Frame frame;
};

}