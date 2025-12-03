#include <gb/memory/serial.h>
#include <gb/utils/log.h>

namespace gb {

namespace {

struct : SerialIO {
	uint8_t handle_serial_transfer(uint8_t value, uint32_t baud) final {
		log_error("Ignoring unimplemented request to shift out {:2x} at {} Hz", value, baud);
		return 0xFF;
	}
} SerialIODisconnected;

}

SerialIO& SERIAL_DISCONNECTED = SerialIODisconnected; 

}