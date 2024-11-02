#pragma once

namespace gb::consts {

constexpr double TCLK_HZ = 1 << 22; // accurate to a few ppm
constexpr double MCLK_HZ = TCLK_HZ / 4;

}