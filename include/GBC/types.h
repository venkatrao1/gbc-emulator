#include <cstdint>

namespace gb {

// represents an address, according to the gb cpu.
// 16 bits.
struct Addr {
	Addr() = delete;
	constexpr Addr(uint16_t raw_in) : raw{raw_in} {}

	constexpr Addr operator+(uint16_t offs) const {
		return Addr{static_cast<uint16_t>(raw+offs)};
	}
	constexpr Addr operator-(uint16_t offs) const {
		return *this + (UINT16_MAX - offs);
	}
	constexpr uint16_t operator-(Addr other) const {
		return (*this - other.raw).raw;
	}

	// TODO replace with spaceship operator C++20
	constexpr bool operator<(const Addr& other) const { return raw < other.raw; }
	constexpr bool operator>(const Addr& other) const { return raw > other.raw; }
	constexpr bool operator==(const Addr& other) const { return raw == other.raw; }
	constexpr bool operator<=(const Addr& other) const { return raw <= other.raw; }
	constexpr bool operator>=(const Addr& other) const { return raw >= other.raw; }
	constexpr bool operator!=(const Addr& other) const { return raw != other.raw; }

	uint16_t raw;
};

}