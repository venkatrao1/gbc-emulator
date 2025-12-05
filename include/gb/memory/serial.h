#pragma once

#include <gb/utils/log.h>

#include <cstdint>

namespace gb {

struct SerialIO {
	// Shift a byte into this SerialIO, return the byte that will be shifted out.
	virtual uint8_t handle_serial_transfer(uint8_t value, uint32_t baud) = 0;
};

constexpr uint8_t SERIAL_DISCONNECTED_VALUE = 0xFF;

extern SerialIO& SERIAL_DISCONNECTED;

}