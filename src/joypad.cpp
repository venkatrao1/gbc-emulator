#include <gb/joypad.h>
#include <gb/utils/log.h>

#include <array>
#include <cstdint>
#include <format>

namespace gb::joypad {

uint8_t Joypad::read_nybble(bool read_buttons, bool read_dpad) const {
    uint8_t ret = 0xF;
    if(read_dpad) ret &= values;
    if(read_buttons) ret &= values >> 4;
    return ret;
}

void Joypad::press(joypad_bits pressed) {
    log_debug("pressed {}", pressed);
    values &= ~(1 << static_cast<uint8_t>(pressed));
}

void Joypad::release(joypad_bits released) {
    log_debug("released {}", released);
    values |= 1 << static_cast<uint8_t>(released);
}

} // ns gb::joypad

auto std::formatter<gb::joypad::joypad_bits>::format(gb::joypad::joypad_bits i, auto& ctx) const {
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