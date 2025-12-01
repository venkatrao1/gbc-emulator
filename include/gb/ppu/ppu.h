#pragma once

#include "consts.h"
#include <gb/memory/mmu.h>

#include <span>
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
		// TODO: some ram is inaccessible during certain parts (e.g. OAM, VRAM, but not DMG palettes), so can just load it once
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
			if(const unsigned cur_x = line_clks - MODE2_TCLKS; cur_x < LCD_WIDTH) {
				// draw a pixel - for now only bg, ignoring delay
				const auto sprite_px = process_sprite_fifo_px(static_cast<uint8_t>(cur_x + 8), LCDC);
				
				uint8_t bg_palette_color = TRANSPARENT;
				const bool enable_bg_window = LCDC & 1;
				if(enable_bg_window) {
					const int window_x = static_cast<int>(cur_x) + 7 - static_cast<int>(lcd_window_x());
					const int window_y = static_cast<int>(lcd_cur_y()) - static_cast<int>(lcd_window_y());
					
					uint8_t x, y; // tilemap x/y
					bool tile_map;
					if(get_bit(LCDC, 5) && window_x >= 0 && window_y >= 0) {
						x = static_cast<uint8_t>(window_x);
						y = static_cast<uint8_t>(window_y);
						tile_map = get_bit(LCDC, 6);
					} else {
						x = lcd_scroll_x() + static_cast<uint8_t>(cur_x);
						y = lcd_scroll_y() + lcd_cur_y();
						tile_map = get_bit(LCDC, 3);
					}
					
					const uint8_t* tilemap_begin = mmu.vram_begin() + 0x1800 + (tile_map * 0x400); // 0x9800 if 0, 0x9C00 if 1
					const uint8_t tile_idx_raw = tilemap_begin[(static_cast<uint16_t>(y >> 3) << 5) | (x >> 3)];
					const uint8_t* bg_tiledata = mmu.vram_begin(); // 0x8000, LCDC.4 handled below
					const uint16_t tile_idx = tile_idx_raw + (((~LCDC & 0b1'0000) << 4) & ((~tile_idx_raw & 0x80) << 1)); // add 256 if ~LCDC.4 and tile_idx >= 0;
					const uint8_t* bg_tilerow = bg_tiledata + (((tile_idx * TILE_SZ) + (y & 7)) * 2);
					bg_palette_color = ((bg_tilerow[0] >> (7-(x & 7))) & 1) | (((bg_tilerow[1] >> (7-(x & 7))) & 1) << 1);
				}

				constexpr static auto get_palette_color = [](uint8_t palette, uint8_t color) { return static_cast<uint8_t>((palette >> (2 * color)) & 3);};
				Gray display_color{.raw = 0}; // default to white
				if(sprite_px.color && (!sprite_px.low_prio || (bg_palette_color == 0))) {
					const auto sprite_palette = obj_palettes()[sprite_px.palette];
					display_color.raw = get_palette_color(sprite_palette, sprite_px.color);
				} else if(bg_palette_color || enable_bg_window) {
					display_color.raw = get_palette_color(bg_palette_data(), bg_palette_color);
				}

				// if sprite opaque and not low prio, use that.
				// elif background color 1-3, use that.
				// elif sprite opaque and low prio, use that.
				// else render background color 0 if bg_enabled, or white otherwise.

				// TODO: mixing here
				frame[lcd_cur_y()][cur_x] = display_color;
				if(cur_x == LCD_WIDTH - 1) next_mode = Mode::HBLANK;
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
			if(line_clks == MODE2_TCLKS && cur_mode == Mode::RD_OAM) {
				next_mode = Mode::DRAW;
				// simulate OAM scan. TODO handle DMA during mode 2?
				const int obj_height = TILE_SZ + ((LCDC & 0b100) << 1);
				remaining_objects = 0;
				if(get_bit(LCDC, 1)) { // TODO: when does LCDC.1 actually have effects?
					const auto& oam = this->oam();
					for(uint8_t idx = 0; idx < oam.size(); ++idx) {
						const auto& oam_entry = oam[idx];
						const auto sprite_row = lcd_cur_y() - (oam_entry.y_plus_16 - 16);
						if(sprite_row < 0 || sprite_row >= obj_height) continue;
						scanned_objects[remaining_objects] = {
							.idx = idx,
							.x_plus_8 = oam_entry.x_plus_8,
							.row_ignoring_flip = static_cast<uint8_t>(sprite_row),
						};
						++remaining_objects;
						if(remaining_objects == scanned_objects.size()) break;
					}
				}
				std::sort(scanned_objects.begin(), scanned_objects.begin()+remaining_objects, [](scanned_object a, scanned_object b){
					if(a.x_plus_8 == b.x_plus_8) return a.idx > b.idx;
					return a.x_plus_8 > b.x_plus_8;
				});
				// don't need to clear sprite fifo here because first 8 px are garbage anyway.
				for(uint8_t x_plus_8 = 0; x_plus_8 < 8; ++x_plus_8) {
					process_sprite_fifo_px(x_plus_8, LCDC);
				}
			}
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
	[[nodiscard]] auto obj_palettes() const { return std::span<const uint8_t, 2>{&mmu.get<memory::addrs::OBJ_PALETTE0_DATA>(), 2}; };
	[[nodiscard]] const uint8_t& obj_palette0_data() const { return mmu.get<memory::addrs::OBJ_PALETTE0_DATA>(); }
	[[nodiscard]] const uint8_t& obj_palette1_data() const { return mmu.get<memory::addrs::OBJ_PALETTE1_DATA>(); };
	[[nodiscard]] const uint8_t& lcd_window_y() const { return mmu.get<memory::addrs::LCD_WINDOW_Y>(); };
	[[nodiscard]] const uint8_t& lcd_window_x() const { return mmu.get<memory::addrs::LCD_WINDOW_X>(); };
	
	// TODO - would need testing on real hardware
	struct scanned_object {
		uint8_t idx;
		uint8_t x_plus_8;
		uint8_t row_ignoring_flip;
	};
	std::array<scanned_object, 10> scanned_objects; // sorted in reverse priority so we can pop from back
	uint8_t remaining_objects = 0;

	struct sprite_fifo_px {
		uint8_t color : 2 = TRANSPARENT;
		uint8_t palette: 1;
		bool low_prio: 1;
		// NOTE: on DMG only color is considered, on CGB we should also default to lower than lowest prio here.
	};
	std::array<sprite_fifo_px, 8> sprite_fifo{};
	uint8_t sprite_fifo_head = 0;

	// check for sprite(s) at current pixel,and load into fifo, then pop pixel off fifo
	sprite_fifo_px process_sprite_fifo_px(uint8_t x_plus_8, uint8_t LCDC) {
		const auto oam = this->oam();
		while(remaining_objects > 0 && scanned_objects[remaining_objects-1].x_plus_8 == x_plus_8) {
			const auto scanned_obj = scanned_objects[remaining_objects-1];
			const oam_entry obj = oam[scanned_obj.idx];
			const int obj_height = TILE_SZ + ((LCDC & 0b100) << 1);
			const bool low_prio = get_bit(obj.flags, 7);
			const bool y_flip = get_bit(obj.flags, 6);
			const bool x_flip = get_bit(obj.flags, 5);
			const bool palette = get_bit(obj.flags, 4);
			auto y = scanned_obj.row_ignoring_flip;
			if(y_flip) {
				y ^= (obj_height - 1);
			}
			const auto base_tile_idx = obj.tile_idx & ~((LCDC >> 2) & 1); // if LCDC 1, zero last bit of idx
			// TODO: what happens if we have change to using short objects in the middle of a frame and read past end?
			const auto tiledata = mmu.vram_begin() + 2*(TILE_SZ*base_tile_idx + y);

			uint8_t tiledata_lo = tiledata[0];
			uint8_t tiledata_hi = tiledata[1]; 
			for(uint8_t i = 0; i<8; ++i) {
				const auto idx = (sprite_fifo_head + ((~-static_cast<int>(x_flip)) ^ i)) & 7; // LSB is rightmost, so i should be unchanged when x_flip is true
				if(sprite_fifo[idx].color == TRANSPARENT) {
					sprite_fifo[idx] = {
						.color = static_cast<uint8_t>(((tiledata_hi & 1) << 1) | (tiledata_lo & 1)),
						.palette = palette,
						.low_prio = low_prio,
					};
				}
				tiledata_lo >>= 1;
				tiledata_hi >>= 1;
			}

			--remaining_objects;
		}

		// pop from queue: clear/advance head
		auto& head = sprite_fifo[sprite_fifo_head & 7];
		const auto ret = head;
		head = {};
		sprite_fifo_head++; 
		return ret;
	}

	std::span<const oam_entry, NUM_OAM_ENTRIES> oam() const {
		const auto& oam_view = mmu.oam_view();
		return std::span<const oam_entry, NUM_OAM_ENTRIES>{
			reinterpret_cast<const oam_entry*>(oam_view.data()),
			reinterpret_cast<const oam_entry*>(oam_view.data()+oam_view.size())
		};
	}


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