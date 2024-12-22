#pragma once

#include <array>
#include <cstdint>
#include <format>

namespace gb::joypad {

enum class joypad_bits : uint8_t {
    // dpad
    dpad_right = 0,
    dpad_left,
    dpad_up,
    dpad_down,
    // other (MMU bit index + 4)
    a,
    b,
    select,
    start
};

struct Joypad {
    // read as laid out in memory
    uint8_t read_nybble(bool read_buttons, bool read_dpad) const;

    // for debug - returns 1 if pressed, unlike memory
    bool read_button(joypad_bits) const;

    void press(joypad_bits pressed);

    void release(joypad_bits released);

private:
    uint8_t values{0xFF};
};

} // ns gb::joypad

template<>
struct std::formatter<gb::joypad::joypad_bits> : std::formatter<std::string_view> {
	auto format(gb::joypad::joypad_bits i, auto& ctx) const {
        using namespace std::string_view_literals;
        constexpr static std::array names{
            "right"sv,
            "left"sv,
            "up"sv,
            "down"sv,
            "a"sv,
            "b"sv,
            "select"sv,
            "start"sv,
        };
        const auto raw = static_cast<uint8_t>(i);
        const auto name = raw < names.size() ? names[raw] : "UNKNOWN"sv;
        return std::formatter<std::string_view>::format(name, ctx);
    }
};