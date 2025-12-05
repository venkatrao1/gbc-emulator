#pragma once

namespace gb::consts {

constexpr double TCLK_HZ = 1 << 22; // accurate to a few ppm	

}

namespace gb::ppu {

constexpr unsigned VBLANK_LINES = 10;
constexpr unsigned LINE_TCLKS = 456;

constexpr unsigned LCD_WIDTH = 160;
constexpr unsigned LCD_HEIGHT = 144;
constexpr double FRAME_HZ = consts::TCLK_HZ / (LINE_TCLKS * (LCD_HEIGHT+VBLANK_LINES));

}