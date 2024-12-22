#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace gb {

template<std::unsigned_integral T>
[[nodiscard]] constexpr T mask_combine(const std::type_identity_t<T> mask, const T in0, const T in1) {
	return (mask & in1) | (~mask & in0);
}

[[nodiscard]] constexpr bool get_bit(std::unsigned_integral auto in, uint8_t bit_idx) {
	return (in >> bit_idx) & 1;
}

constexpr void set_bit(std::unsigned_integral auto& in, uint8_t bit_idx) {
	in |= (1ULL << bit_idx);
}

constexpr void rst_bit(std::unsigned_integral auto& in, uint8_t bit_idx) {
	in &= ~(1ULL << bit_idx);
}

}