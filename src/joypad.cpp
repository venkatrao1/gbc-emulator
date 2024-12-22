#include <gb/joypad.h>
#include <gb/utils/log.h>

#include <cstdint>

namespace gb::joypad {

uint8_t Joypad::read_nybble(bool read_buttons, bool read_dpad) const {
    uint8_t ret = 0xF;
    if(read_dpad) ret &= values;
    if(read_buttons) ret &= values >> 4;
    return ret;
}

bool Joypad::read_button(joypad_bits button) const {
    return ~values & (1 << static_cast<uint8_t>(button));
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