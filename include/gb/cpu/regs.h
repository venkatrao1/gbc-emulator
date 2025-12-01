#pragma once

#include <cstdint>
#include <format>

namespace gb::cpu {

// af, bc, de, hl are all 2 8-bit registers that can form a 16-bit reg.
// because I don't want to rely on nonstandard aliasing, we can't just stick 'em in a union.
struct alignas(uint16_t) Reg16 {
	constexpr Reg16() = default;
	constexpr Reg16(uint16_t in) : lo{static_cast<uint8_t>(in & 0xFF)}, hi{static_cast<uint8_t>(in >> 8)} {}

	// little-endian storage so hopefully things can be optimized.
	uint8_t lo{};
	uint8_t hi{};

	constexpr operator uint16_t() const { return (hi << 8) | lo; }

	constexpr Reg16& operator +=(const uint16_t b) {
		*this = uint16_t{*this} + b;
		return *this;
	}
	constexpr Reg16& operator -=(const uint16_t b) {
		*this = uint16_t{*this} - b;
		return *this;
	}

	constexpr Reg16& operator++() {
		return (*this += 1);
	}
	constexpr Reg16 operator++(int) {
		const auto ret = *this;
		++(*this);
		return ret;
	}
	constexpr Reg16& operator--() {
		return (*this -= 1);
	}
	constexpr Reg16 operator--(int) {
		const auto ret = *this;
		--(*this);
		return ret;
	}
};

}

template<>
struct std::formatter<gb::cpu::Reg16> : std::formatter<uint16_t> {};