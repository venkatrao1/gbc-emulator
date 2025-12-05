#include <gb/memory/serial.h>
#include <gb/utils/log.h>

namespace gb {

namespace {

struct : SerialIO {
	uint8_t handle_serial_transfer(uint8_t value, uint32_t baud) final {
		log_warn("Ignoring unimplemented request to shift out {:#04x} at {} Hz", value, baud);
		return SERIAL_DISCONNECTED_VALUE;
	}
} SerialIODisconnected;

}

SerialIO& SERIAL_DISCONNECTED = SerialIODisconnected; 

}